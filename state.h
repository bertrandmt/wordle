#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>
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

class WordEntropy {
public:
    inline WordEntropy(const Word &word, double entropy)
        : mWord(word)
        , mEntropy(entropy) { }

    inline WordEntropy()
        : mWord("", false)
        , mEntropy(0) { }

    inline const Word &word() const {
        return mWord;
    }

    inline double entropy() const {
        return mEntropy;
    }

    bool operator<(const WordEntropy &other) const {
        return other.mEntropy < mEntropy; // inverted, to achieve decreasing order
    }

private:
    Word mWord;
    double mEntropy;
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

class State {
public:
    State(ThreadPool &pool, const std::vector<const Word> &all_words);
    State consider_guess(const std::string &guess, uint32_t match, bool do_print = true) const;

    double max_entropy() const;
    inline size_t n_solutions() const {
        return mNSolutions;
    }
    void print() const;

    double entropy_of(const std::string &word) const;
    double entropy2_of(const std::string &word) const;

    void best_guess() const;

private:
    State(ThreadPool &pool, const std::vector<const Word> &all_words, int generation, const std::vector<const Word> &words, const Keyboard &keyboard, bool do_print = true);

    double compute_entropy(const std::string &word) const;
    double compute_entropy2(const std::string &word) const;

    ThreadPool &mPool;

    const int mGeneration;
    const std::vector<const Word> &mAllWords;
    const std::vector<const Word> mWords;
    size_t mNSolutions;

    double mMaxEntropy;
    std::vector<WordEntropy> mEntropy;
    std::vector<WordEntropy> mEntropy2;

    Keyboard mKeyboard;
};
