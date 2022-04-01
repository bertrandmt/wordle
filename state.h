// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "word.h"

class Keyboard;
class StateCache;
class ThreadPool;

class State {
public:
    typedef std::shared_ptr<State> ptr;

    State(ThreadPool &pool, const std::shared_ptr<StateCache> &state_cache, const Words &all_words);
    ptr consider_guess(const std::string &guess, uint32_t match, bool do_full_compute = true) const;
    static ptr unserialize(std::istream &is, const std::shared_ptr<StateCache> &cache);

    inline std::size_t n_words() const { return mWords.size(); }
    const Words &words() const { return mWords; }
    inline std::size_t n_solutions() const { return mNSolutions; }
    const Words &solutions() const { return mSolutions; }

    uint32_t max_entropy() const;
    inline bool is_fully_computed() const { return mFullyComputed; }

    uint32_t entropy_of(const std::string &word) const;
    uint32_t entropy2_of(const std::string &word) const;
    bool words_equal_to(const Words &other_words) const;

    inline std::vector<WordEntropy> solution_entropies() const {
	std::vector<WordEntropy> the_entropies;
        for (auto word : mSolutions) {
            auto it = std::find_if(mEntropy2.begin(), mEntropy2.end(), [word](const WordEntropy &e) { return e.word().word() == word.word(); });
	    if (it == mEntropy2.end()) {
		the_entropies.push_back(WordEntropy(word, 0));
	    }
	    else {
		the_entropies.push_back(*it);
	    }
	}
	return the_entropies;
    }
    std::vector<ScoredEntropy> best_guess(const Keyboard &keyboard) const;

    void serialize(std::ostream &os) const;

private:
    State(const State &other, const Words &filtered_words, bool do_full_compute = true);
    State(const ptr &other, const Words &words, const std::vector<WordEntropy> &entropy, const std::vector<WordEntropy> &entropy2, bool fully_computed);

    uint32_t compute_entropy_of(const std::string &word) const;
    uint32_t compute_entropy2_of(const std::string &word) const;

    void compute_real_entropy() const;

    ThreadPool &mPool;
    std::shared_ptr<StateCache> mStateCache;

    const Words &mAllWords;
    const Words mWords;
    const size_t mNSolutions;
    const Words mSolutions;	// populated only if size will be less than MAX_N_SOLUTIONS_PRINTED

    mutable uint32_t mMaxEntropy;
    mutable std::vector<WordEntropy> mEntropy;
    mutable std::vector<WordEntropy> mEntropy2;
    mutable std::vector<WordEntropy>::const_iterator mHighestEntropy2End;
    mutable bool mFullyComputed;
};
