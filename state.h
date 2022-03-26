// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "keyboard.h"
#include "threadpool.h"

class Word {
public:
    inline Word(std::string word, bool is_solution)
        : mWord(word)
        , mIsSolution(is_solution) { }

    inline Word(const Word &other)
        : mWord(other.mWord)
        , mIsSolution(other.mIsSolution) { }

    inline std::string word() const {
        return mWord;
    }

    inline bool is_solution() const {
        return mIsSolution;
    }

private:
    std::string mWord;
    bool mIsSolution;
};

typedef std::vector<const Word> Words;

class WordEntropy {
public:
    inline WordEntropy(const Word &word, uint32_t entropy)
        : mWord(word)
        , mEntropy(entropy) { }

    inline WordEntropy()
        : mWord("", false)
        , mEntropy(0) { }

    inline const Word &word() const {
        return mWord;
    }

    inline uint32_t entropy() const {
        return mEntropy;
    }

    bool operator<(const WordEntropy &other) const {
        return other.mEntropy < mEntropy; // inverted, to achieve decreasing order
    }

private:
    Word mWord;
    uint32_t mEntropy;
};

class ScoredEntropy {
public:
    ScoredEntropy(const WordEntropy &entropy, const Keyboard &keyboard);

    inline WordEntropy entropy() const {
        return mEntropy;
    }

    inline int score() const {
        return mScore;
    }

    bool operator<(const ScoredEntropy &other) const {
        return other.mScore < mScore; // inverted, to achieve decreasing order
    }

private:
    WordEntropy mEntropy;
    int mScore;
};

class StateCache;

class State {
public:
    State(ThreadPool &pool, StateCache &state_cache, const Words &all_words);
    std::shared_ptr<State> consider_guess(const std::string &guess, uint32_t match, bool do_full_compute = true) const;

    uint32_t max_entropy() const;
    inline std::size_t n_solutions() const {
        return mNSolutions;
    }
    inline std::size_t n_words() const {
        return mWords.size();
    }

    uint32_t entropy_of(const std::string &word) const;
    uint32_t entropy2_of(const std::string &word) const;
    bool words_equal_to(const Words &other_words) const;

    void best_guess(int generation, const Keyboard &keyboard) const;

private:
    State(const State &other, const Words &filtered_words, bool do_full_compute = true);

    uint32_t compute_entropy_of(const std::string &word) const;
    uint32_t compute_entropy2_of(const std::string &word) const;

    void compute_real_entropy() const;

    ThreadPool &mPool;
    StateCache &mStateCache;

    const Words &mAllWords;
    const Words mWords;
    size_t mNSolutions;

    uint32_t mMaxEntropy;
    mutable std::vector<WordEntropy> mEntropy;
    mutable std::vector<WordEntropy> mEntropy2;
    mutable bool mFullyComputed;
};
