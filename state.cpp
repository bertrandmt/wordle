#include <cmath>
#include <iostream>
#include <iterator>
#include <random>

#include "config.h"
#include "match.h"
#include "state.h"

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

State::State(ThreadPool &pool, const std::vector<const Word> &all_words)
    : mPool(pool)
    , mGeneration(1)
    , mAllWords(all_words)
    , mWords(all_words) {

    mNSolutions = std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; });

    print();
}

State::State(ThreadPool &pool, const std::vector<const Word> &all_words, int generation, const std::vector<const Word> &words, const Keyboard &keyboard, bool do_print)
    : mPool(pool)
    , mGeneration(generation)
    , mAllWords(all_words)
    , mWords(words)
    , mMaxEntropy(0)
    , mKeyboard(keyboard) {


    /* 1. compute number of solutions among words */
    mNSolutions = std::transform_reduce(mWords.begin(), mWords.end(), 0, std::plus(), [](const Word &word) -> size_t { return word.is_solution() ? 1 : 0; });

    if (do_print) print();

    /* 2. compute entropy */
    const size_t num_blocks = mPool.num_threads();

    std::mutex lock;
    int ndone = 0;
    std::condition_variable cond;

    if (do_print) {
        std::cout << "Computing entropy..." << std::flush;
        mEntropy.resize(mAllWords.size());
        const size_t block_sz = mEntropy.size() / num_blocks + 1;

        for (auto i = 0; i < num_blocks; i++) {
            pool.push([i, block_sz, this, &lock, &ndone, &cond]() {
                    for (auto j = i * block_sz; j < (i+1) * block_sz && j < mEntropy.size(); j++) {
                        const Word &word = mAllWords.at(j);
                        mEntropy.at(j) = WordEntropy(word, compute_entropy(word.word()));
                    }
                    {
                        std::lock_guard<std::mutex> lk(lock);
                        ndone += 1;
                        std::cout << "." << std::flush;
                    }
                    cond.notify_all();
                });
        }
        {
            std::unique_lock<std::mutex> lk(lock);
            cond.wait(lk, [&ndone, num_blocks]() { return ndone == num_blocks; });
        }
    }
    else {
        // when in submode (not printing), run entropy only on _words_, not _all words_
        std::transform(mWords.begin(), mWords.end(), std::back_inserter(mEntropy), [this](const Word &word) { return WordEntropy(word, compute_entropy(word.word())); });
    }

    /* 3. compute max entropy */
    mMaxEntropy = std::transform_reduce(mEntropy.begin(), mEntropy.end(), 0.0, [](double max_h, double h) { return std::max(h, max_h); },
                                                                               [](const WordEntropy &e) -> double { return e.entropy(); });

    /* 5. sort entropy decreasing */
    std::sort(mEntropy.begin(), mEntropy.end());

    /* 4. compute entropy2 */
    if (do_print) { // this is goofy :(
        mEntropy2.resize(ENTROPY_2_TOP_N);
        const size_t block_sz = mEntropy2.size() / num_blocks + 1;
        ndone = 0;

        for (auto i = 0; i < num_blocks; i++) {
            pool.push([i, block_sz, this, &lock, &ndone, &cond]() {
                    for (auto j = i * block_sz; j < (i+1) * block_sz && j < mEntropy2.size(); j++) {
                        const Word &word = mEntropy.at(j).word();
                        double h = mEntropy.at(j).entropy();
                        mEntropy2.at(j) = WordEntropy(word, h + compute_entropy2(word.word()));
                    }
                    {
                        std::lock_guard<std::mutex> lk(lock);
                        ndone += 1;
                        std::cout << "." << std::flush;
                    }
                    cond.notify_all();
                });
        }
        {
            std::unique_lock<std::mutex> lk(lock);
            cond.wait(lk, [&ndone, num_blocks]() { return ndone == num_blocks; });
        }
        std::cout << std::endl;
    }

    /* 6. sort entropy2 decreasing */
    std::sort(mEntropy2.begin(), mEntropy2.end());
}

State State::consider_guess(const std::string &guess, uint32_t match, bool do_print) const {
    Match m(guess, match);
    if (do_print) {
        std::cout << "Considering guess \"" << guess << "\" with match " << m.toString() << std::endl;
    }

    std::vector<const Word> filtered_words;
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

    Keyboard updated_keyboard = mKeyboard.updateWithGuess(guess, m);

    return State(mPool, mAllWords, mGeneration + 1, filtered_words, updated_keyboard, do_print); // RVO?
}

double State::compute_entropy(const std::string &word) const {
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
    return H;
}

double State::compute_entropy2(const std::string &word) const {
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
        auto H_2 = s.max_entropy();
        double Pxi = (double)match_counts[match] / mNSolutions;
        H += Pxi * H_2;
    }
    return H;
}

double State::max_entropy() const {
    return mMaxEntropy;
}

void State::print() const {
    std::cout << "State[gen#" << mGeneration << "]: " << mNSolutions << " solutions and " << mWords.size() << " words." << std::endl;
    mKeyboard.print();
}

double State::entropy_of(const std::string &word) const {
    auto it = std::find_if(mEntropy.begin(), mEntropy.end(), [word](const WordEntropy &e){ return e.word().word() == word; });
    if (it == mEntropy.end()) return 0;

    return it->entropy();
}

double State::entropy2_of(const std::string &word) const {
    auto it = std::find_if(mEntropy2.begin(), mEntropy2.end(), [word](const WordEntropy &e){ return e.word().word() == word; });
    if (it == mEntropy2.end()) return 0;

    return it->entropy();
}

ScoredEntropy::ScoredEntropy(const WordEntropy &entropy, const Keyboard &keyboard)
    : mEntropy(entropy)
    , mScore(0) {

    for (char c : mEntropy.word().word()) {
        const Letter &l = keyboard.letter(c);
        mScore += static_cast<int>(l.state());
    }
}

void State::best_guess() const {
    /* stop! */
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

    if (mNSolutions <= 15) {
        std::cout << "Solutions and associated entropy:" << std::endl;
        for (auto word : mWords) {
            if (word.is_solution()) {
                auto it = std::find_if(mEntropy2.begin(), mEntropy2.end(), [word](const WordEntropy &e) { return e.word().word() == word.word(); });
                auto h = (it == mEntropy2.end()) ? 0.0 : it->entropy();

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
        scored_entropy.push_back(ScoredEntropy(*jt, mKeyboard));
    }
    std::sort(scored_entropy.begin(), scored_entropy.end());

    auto scored_recommended_guesses_end = scored_entropy.begin();
    while (scored_entropy.front().score() == scored_recommended_guesses_end->score()) {
        scored_recommended_guesses_end++;
    }
    auto scored_recommended_guesses_size = std::distance(scored_entropy.begin(), scored_recommended_guesses_end);

    std::cout << "Recommending guess \"" << select_randomly(scored_entropy.begin(), scored_recommended_guesses_end)->entropy().word().word() << "\" out of " << scored_recommended_guesses_size << " words ";
    std::cout << "(";
    bool first = true;
    for (auto it = scored_entropy.begin(); it != scored_recommended_guesses_end && distance(scored_entropy.begin(), it) < 15; it++) {
        if (!first) std::cout << ", ";
        first = false;
        std::cout << "\"" << it->entropy().word().word() << "\"";
    }
    std::cout << ") ";
    std::cout << "with highest two-level entropy " << scored_entropy.front().entropy().entropy()
              << " and highest score " << scored_entropy.front().score() << "." << std::endl;
}
