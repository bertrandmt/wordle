// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <iterator>
#include <mutex>
#include <random>

#include "config.h"
#include "match.h"
#include "state.h"
#include "statecache.h"

namespace {

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

} // namespace anonymous

State::State(ThreadPool &pool, const StateCache::ptr &state_cache, const Words &all_words)
    : mPool(pool)
    , mStateCache(state_cache)
    , mAllWords(all_words)
    , mWords(all_words)
    , mFullyComputed(false) {

    mNSolutions = std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; });
}

void State::compute_real_entropy() const {
    std::mutex lock;
    int ndone = 0;
    std::condition_variable cond;

    const size_t num_blocks = mPool.num_threads();

#if DEBUG_ENTROPY
    std::cout << "Computing entropy..." << std::flush;
#endif // DEBUG_ENTROPY
    mEntropy.resize(mAllWords.size());
    size_t block_sz = mEntropy.size() / num_blocks + 1;

    /* 1. compute entropy */
    for (auto i = 0; i < num_blocks; i++) {
        mPool.push([i, block_sz, this, &lock, &ndone, &cond]() {
                for (auto j = i * block_sz; j < (i+1) * block_sz && j < mEntropy.size(); j++) {
                    const Word &word = mAllWords.at(j);
                    mEntropy.at(j) = WordEntropy(word, compute_entropy_of(word.word()));
                }
                {
                    std::lock_guard<std::mutex> lk(lock);
                    ndone += 1;
#if DEBUG_ENTROPY
                    std::cout << "." << std::flush;
#endif // DEBUG_ENTROPY
                }
                cond.notify_all();
            });
    }
    {
        std::unique_lock<std::mutex> lk(lock);
        cond.wait(lk, [&ndone, num_blocks]() { return ndone == num_blocks; });
    }

    /* 2. sort entropy decreasing */
    std::sort(mEntropy.begin(), mEntropy.end());

    /* 3. compute entropy2 */
    mEntropy2.resize(ENTROPY_2_TOP_N);
    block_sz = mEntropy2.size() / num_blocks + 1;
    ndone = 0;

    for (auto i = 0; i < num_blocks; i++) {
        mPool.push([i, block_sz, this, &lock, &ndone, &cond]() {
                for (auto j = i * block_sz; j < (i+1) * block_sz && j < mEntropy2.size(); j++) {
                    const Word &word = mEntropy.at(j).word();
                    uint32_t h = mEntropy.at(j).entropy();
                    mEntropy2.at(j) = WordEntropy(word, h + compute_entropy2_of(word.word()));
                }
                {
                    std::lock_guard<std::mutex> lk(lock);
                    ndone += 1;
#if DEBUG_ENTROPY
                    std::cout << "." << std::flush;
#endif // DEBUG_ENTROPY
                }
                cond.notify_all();
            });
    }
    {
        std::unique_lock<std::mutex> lk(lock);
        cond.wait(lk, [&ndone, num_blocks]() { return ndone == num_blocks; });
    }
#if DEBUG_ENTROPY
    std::cout << std::endl;
#endif // DEBUG_ENTROPY

    /* 4. sort entropy2 decreasing */
    std::sort(mEntropy2.begin(), mEntropy2.end());
}

State::State(const State &other, const Words &filtered_words, bool do_full_compute)
    : mPool(other.mPool)
    , mStateCache(other.mStateCache)
    , mAllWords(other.mAllWords)
    , mWords(filtered_words)
    , mMaxEntropy(0)
    , mFullyComputed(do_full_compute) {

    /* 1. compute number of solutions among words */
    mNSolutions = std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; });

    /* 2. compute entropies */
    if (do_full_compute) {
        compute_real_entropy();
    }
    else {
        // when in submode (not printing), run entropy only on _words_, not _all words_
        std::transform(mWords.begin(), mWords.end(), std::back_inserter(mEntropy), [this](const Word &word) { return WordEntropy(word, compute_entropy_of(word.word())); });
    }

    /* 3. compute max entropy */
    mMaxEntropy = std::transform_reduce(mEntropy.begin(), mEntropy.end(), 0, [](uint32_t max_h, uint32_t h) { return std::max(h, max_h); },
                                                                             [](const WordEntropy &e) -> uint32_t { return e.entropy(); });
}

State::ptr State::consider_guess(const std::string &guess, uint32_t match, bool do_full_compute) const {
    Match m(guess, match);
    if (do_full_compute) {
        std::cout << "Considering guess \"" << guess << "\" with match " << m.toString() << std::endl;
    }

    Words filtered_words;
    std::copy_if(mWords.begin(), mWords.end(),
            std::back_inserter(filtered_words),
            [guess, match](const Word &w) {
                Match n(guess, w.word());
#if DEBUG_ACCEPT_WORDS
                if (n.value() == match) { std::cout << "Accepting word \"" << w.word() << "\" with match " << n.toString() << std::endl; }
#endif
#if DEBUG_REJECT_WORDS
                std::cout << "Considering word \"" << w.word() << "\" with match " << n.toString() << ": " << (n.value() == m.value() ? "accept" : "reject") << std::endl;
#endif
               return n.value() == match;
            });

    if (mStateCache->contains(filtered_words)) {
#if DEBUG_STATE_CACHE
        std::cout << "+" << std::flush;
#endif // DEBUG_STATE_CACHE

        return mStateCache->at(filtered_words);
    }
    else {
#if DEBUG_STATE_CACHE
        std::cout << "-" << std::flush;
#endif // DEBUG_STATE_CACHE

        State::ptr s(new State(*this, filtered_words, do_full_compute));
        auto jt = mStateCache->insert(filtered_words, s);
        return jt.first->second;
    }
}

uint32_t State::compute_entropy_of(const std::string &word) const {
    std::vector<uint32_t> match_counts(kMaxMatchValue + 1, 0);

    for (auto solution : mWords) {
        if (!solution.is_solution()) continue;

        Match m(word, solution.word());
        match_counts[m.value()]++;
    }

    double H = 0;
    for (auto cnt : match_counts) {
        if (cnt == 0) continue;
        double Pxi = (double)cnt / mNSolutions;
        H -= Pxi * std::log(Pxi);
    }
    return static_cast<uint32_t>(H * 1000);
}

uint32_t State::compute_entropy2_of(const std::string &word) const {
    std::vector<uint32_t> match_counts(kMaxMatchValue + 1, 0);

    for (auto solution : mWords) {
        if (!solution.is_solution()) continue;

        Match m(word, solution.word());
        match_counts[m.value()]++;
    }

    double H = 0;
    for (auto match = 0; match < match_counts.size(); match++) {
        if (match_counts[match] == 0) continue;

        auto s = consider_guess(word, match, false);
        auto H_2 = s->max_entropy();
        double Pxi = (double)match_counts[match] / mNSolutions;
        H += Pxi * H_2;
    }
    return static_cast<uint32_t>(H);
}

uint32_t State::max_entropy() const {
    return mMaxEntropy;
}

uint32_t State::entropy_of(const std::string &word) const {
    auto it = std::find_if(mEntropy.begin(), mEntropy.end(), [word](const WordEntropy &e){ return e.word().word() == word; });
    if (it == mEntropy.end()) return 0;

    return it->entropy();
}

uint32_t State::entropy2_of(const std::string &word) const {
    auto it = std::find_if(mEntropy2.begin(), mEntropy2.end(), [word](const WordEntropy &e){ return e.word().word() == word; });
    if (it == mEntropy2.end()) return 0;

    return it->entropy();
}

bool State::words_equal_to(const Words &other_words) const {
    return std::equal_to<Words>{}(mWords, other_words);
}

ScoredEntropy::ScoredEntropy(const WordEntropy &entropy, const Keyboard &keyboard)
    : mEntropy(entropy)
    , mScore(0) {

    bool seen['z'-'a'] = { false };
    for (char c : mEntropy.word().word()) {
        const Letter &l = keyboard.letter(c);
        if (!seen[c - 'a']) {
            mScore += static_cast<int>(l.state());
            seen[c - 'a'] = true;
        }
    }
}

void State::best_guess(int generation, const Keyboard &keyboard) const {
    /* stop! */
    if (generation == 1) {
        std::cout << "Initial best guess is \"trace\"." << std::endl;
        return;
    }
    if (mNSolutions == 0) {
        std::cout << "No solution left 😭" << std::endl;
        return;
    }
    if (mNSolutions == 1) {
        auto it = std::find_if(mWords.begin(), mWords.end(), [](const Word &w){ return w.is_solution(); });
        assert(it != mWords.end());
        std::cout << "THE solution: \"" << it->word() << "\"" << std::endl;
        return;
    }

    if (!mFullyComputed) {
        compute_real_entropy();
        mFullyComputed = true;
    }

    if (mNSolutions <= MAX_N_SOLUTIONS_PRINTED) {
        std::cout << "Solutions and associated entropy:" << std::endl;
        for (auto word : mWords) {
            if (word.is_solution()) {
                auto it = std::find_if(mEntropy2.begin(), mEntropy2.end(), [word](const WordEntropy &e) { return e.word().word() == word.word(); });
                auto h = (it == mEntropy2.end()) ? 0.0 : it->entropy() / 1000.;

                std::cout << "\t\"" << word.word() << "\": " << h << std::endl;
            }
        }
    }

    auto recommended_guesses_end = mEntropy2.begin();
    while (mEntropy2.front().entropy() == recommended_guesses_end->entropy()) {
        recommended_guesses_end++;
    }

    std::vector<ScoredEntropy> scored_entropy;
    for (auto jt = mEntropy2.begin(); jt != recommended_guesses_end; jt++) {
        scored_entropy.push_back(ScoredEntropy(*jt, keyboard));
    }
    std::sort(scored_entropy.begin(), scored_entropy.end());

    auto scored_recommended_guesses_end = scored_entropy.begin();
    while (scored_entropy.front().score() == scored_recommended_guesses_end->score()) {
        scored_recommended_guesses_end++;
    }
    auto scored_recommended_guesses_size = std::distance(scored_entropy.begin(), scored_recommended_guesses_end);

    std::cout << "[H=" << scored_entropy.front().entropy().entropy() / 1000. << "|S=" << scored_entropy.front().score()
              << "] \"" << select_randomly(scored_entropy.begin(), scored_recommended_guesses_end)->entropy().word().word() << "\" (" << scored_recommended_guesses_size << " words: ";
    bool first = true;
    for (auto it = scored_entropy.begin(); it != scored_recommended_guesses_end && distance(scored_entropy.begin(), it) < MAX_N_GUESSES_PRINTED; it++) {
        if (!first) std::cout << ", ";
        first = false;
        std::cout << "\"" << it->entropy().word().word() << "\"";
    }
    std::cout << ") " << std::endl;
}

void State::serialize(std::ostream & os) const {
    os << mFullyComputed << " "
       << mWords.size() << " ";
    std::for_each(mWords.begin(), mWords.end(), [&os](const Word &w) { w.serialize(os); os << " "; });
    os << mEntropy.size() << " ";
    std::for_each(mEntropy.begin(), mEntropy.end(), [&os](const WordEntropy &e) { e.serialize(os); os << " "; });
    if (mFullyComputed) {
        os << mEntropy2.size() << " ";
        std::for_each(mEntropy2.begin(), mEntropy2.end(), [&os](const WordEntropy &e) { e.serialize(os); os << " "; });
    }
}

State::State(const State::ptr &other, const Words &words, const std::vector<WordEntropy> &entropy, const std::vector<WordEntropy> &entropy2, bool fully_computed)
    : mPool(other->mPool)
    , mStateCache(other->mStateCache)
    , mAllWords(other->mAllWords)
    , mWords(words)
    , mEntropy(entropy)
    , mEntropy2(entropy2)
    , mFullyComputed(fully_computed) {

    mNSolutions = std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; });

    mMaxEntropy = std::transform_reduce(mEntropy.begin(), mEntropy.end(), 0, [](uint32_t max_h, uint32_t h) { return std::max(h, max_h); },
                                                                             [](const WordEntropy &e) -> uint32_t { return e.entropy(); });
}

State::ptr State::unserialize(std::istream &is, const StateCache::ptr &cache) {
    bool fully_computed = false;
    std::size_t n_words = 0;

    is >> fully_computed >> n_words;

    Words words;
    for (std::size_t i = 0; i < n_words; i++) {
        words.push_back(Word::unserialize(is));
    }

    std::size_t n_entropy;
    is >> n_entropy;

    std::vector<WordEntropy> entropy;
    for (std::size_t i = 0; i < n_entropy; i++) {
        entropy.push_back(WordEntropy::unserialize(is));
    }

    std::vector<WordEntropy> entropy2;
    if (fully_computed) {
        std::size_t n_entropy2;
        is >> n_entropy2;

        for (std::size_t i = 0; i < n_entropy2; i++) {
            entropy2.push_back(WordEntropy::unserialize(is));
        }
    }

    return State::ptr(new State(cache->initial_state(), words, entropy, entropy2, fully_computed));
}
