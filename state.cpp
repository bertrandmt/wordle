// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>

#include "config.h"
#include "keyboard.h"
#include "match.h"
#include "state.h"
#include "statecache.h"
#include "threadpool.h"

std::ostream& operator<<(std::ostream& out, const Word& word) {
    return out << "\"" << word.word() << "\"[" << (word.is_solution() ? 'T' : 'F') << "]";
}

std::ostream& operator<<(std::ostream& out, const WordEntropy& word_entropy) {
    return out << word_entropy.word() << "[H=" << word_entropy.entropy() / 1000. << "]";
}

std::ostream& operator<<(std::ostream& out, const ScoredEntropy& scored_entropy) {
    return out << scored_entropy.entropy() << "[S=" << scored_entropy.score() << "]";
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

namespace {

Words extract_solutions(const std::size_t n_solutions, const Words &words) {
    Words the_solutions;

    if (n_solutions > MAX_N_SOLUTIONS_PRINTED) {
        return the_solutions;
    }

    std::remove_copy_if(words.begin(), words.end(), std::back_inserter(the_solutions), [](const Word &word) { return !word.is_solution(); });
    assert(the_solutions.size() == n_solutions);

    return the_solutions;
}

} // namespace anonymous

State::State(ThreadPool &pool, const StateCache::ptr &state_cache, const Words &all_words)
    : mPool(pool)
    , mStateCache(state_cache)
    , mAllWords(all_words)
    , mWords(all_words)
    , mNSolutions(std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; }))
    , mSolutions(extract_solutions(mNSolutions, mWords))
    , mFullyComputed(false) { }

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

    mMaxEntropy = mEntropy.at(0).entropy();

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

    /* 5. find the end of the highest entropy set */
    mHighestEntropy2End = mEntropy2.begin();
    while (mEntropy2.front().entropy() == mHighestEntropy2End->entropy() && mHighestEntropy2End != mEntropy2.end()) {
        mHighestEntropy2End++;
    }

    /* 6. this state is now fully computed! */
    mFullyComputed = true;
}

State::State(const State &other, const Words &filtered_words, bool do_full_compute)
    : mPool(other.mPool)
    , mStateCache(other.mStateCache)
    , mAllWords(other.mAllWords)
    , mWords(filtered_words)
    , mNSolutions(std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; }))
    , mSolutions(extract_solutions(mNSolutions, mWords))
    , mMaxEntropy(0)
    , mFullyComputed(do_full_compute) {

    if (do_full_compute) {
        compute_real_entropy();
    }
    else {
        // when in submode (not printing), run entropy only on _words_, not _all words_
        std::transform(mWords.begin(), mWords.end(), std::back_inserter(mEntropy), [this](const Word &word) { return WordEntropy(word, compute_entropy_of(word.word())); });

        mMaxEntropy = std::transform_reduce(mEntropy.begin(), mEntropy.end(), 0, [](uint32_t max_h, uint32_t h) { return std::max(h, max_h); },
                                                                             [](const WordEntropy &e) -> uint32_t { return e.entropy(); });
    }
}

State::ptr State::consider_guess(const std::string &guess, uint32_t match, bool do_full_compute) const {
    Match m(guess, match);

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

std::vector<ScoredEntropy> State::best_guess(const Keyboard &keyboard) const {
    std::vector<ScoredEntropy> best_guesses;

    /* stop! */
    if (mNSolutions == 0) {
	assert(best_guesses.size() == 0);
        return best_guesses;
    }
    if (mNSolutions == 1) {
	WordEntropy we(mSolutions.at(0), 0);
	ScoredEntropy se(we, 0);
	best_guesses.push_back(se);

	assert(best_guesses.size() == 1);
        return best_guesses;
    }

    if (!mFullyComputed) {
        compute_real_entropy();
    }

    std::vector<ScoredEntropy> scored_entropy;
    for (auto jt = mEntropy2.begin(); jt != mHighestEntropy2End; jt++) {
        scored_entropy.push_back(ScoredEntropy(*jt, keyboard));
    }
    std::sort(scored_entropy.begin(), scored_entropy.end());

    auto scored_recommended_guesses_end = scored_entropy.begin();
    while (scored_entropy.front().score() == scored_recommended_guesses_end->score()) {
        scored_recommended_guesses_end++;
    }

    best_guesses = std::vector<ScoredEntropy>(scored_entropy.begin(), scored_recommended_guesses_end);
    return best_guesses;
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
    , mNSolutions(std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; }))
    , mSolutions(extract_solutions(mNSolutions, mWords))
    , mEntropy(entropy)
    , mEntropy2(entropy2)
    , mFullyComputed(fully_computed) {

    mMaxEntropy = std::transform_reduce(mEntropy.begin(), mEntropy.end(), 0, [](uint32_t max_h, uint32_t h) { return std::max(h, max_h); },
                                                                             [](const WordEntropy &e) -> uint32_t { return e.entropy(); });

    if (mFullyComputed) {
        mHighestEntropy2End = mEntropy2.begin();
        while (mEntropy2.front().entropy() == mHighestEntropy2End->entropy()) {
            mHighestEntropy2End++;
        }
    }
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
