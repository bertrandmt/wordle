#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "threadpool.h"

class Word {
public:
    Word(std::string word, bool is_solution)
        : mWord(word)
        , mIsSolution(is_solution) { }

    Word(const Word &other)
        : mWord(other.mWord)
        , mIsSolution(other.mIsSolution) { }

    std::string word() const {
        return mWord;
    }

    bool is_solution() const {
        return mIsSolution;
    }

private:
    std::string mWord;
    bool mIsSolution;
};

class WordEntropy {
public:
    WordEntropy(const Word &word, double entropy)
        : mWord(word)
        , mEntropy(entropy) { }

    WordEntropy()
        : mWord("", false)
        , mEntropy(0) { }

    const Word &word() const {
        return mWord;
    }

    double entropy() const {
        return mEntropy;
    }

    bool operator<(const WordEntropy &other) const {
        return other.mEntropy < mEntropy; // inverted, to achieve decreasing order
    }

private:
    Word mWord;
    double mEntropy;
};

class Letter {
public:
    enum State : int {
        kAbsent = 0,
        kPresent = 1,
        kUntested = 3
    };

    inline Letter(char value)
        : mValue(value)
        , mState(kUntested) { }

    inline Letter(const Letter &other, State state)
        : mValue(other.mValue)
        , mState(state) { }

    inline Letter(const Letter &other)
        : mValue(other.mValue)
        , mState(other.mState) { }

    inline char value() const {
        return mValue;
    }

    inline State state() const {
        return mState;
    }

private:
    const char mValue;
    const State mState;
};

class Keyboard {
public:
    Keyboard() {
        mLetters.push_back(Letter('q'));
        mLetters.push_back(Letter('w'));
        mLetters.push_back(Letter('e'));
        mLetters.push_back(Letter('r'));
        mLetters.push_back(Letter('t'));
        mLetters.push_back(Letter('y'));
        mLetters.push_back(Letter('u'));
        mLetters.push_back(Letter('i'));
        mLetters.push_back(Letter('o'));
        mLetters.push_back(Letter('p'));
        mLetters.push_back(Letter('a'));
        mLetters.push_back(Letter('s'));
        mLetters.push_back(Letter('d'));
        mLetters.push_back(Letter('f'));
        mLetters.push_back(Letter('g'));
        mLetters.push_back(Letter('h'));
        mLetters.push_back(Letter('j'));
        mLetters.push_back(Letter('k'));
        mLetters.push_back(Letter('l'));
        mLetters.push_back(Letter('z'));
        mLetters.push_back(Letter('x'));
        mLetters.push_back(Letter('c'));
        mLetters.push_back(Letter('v'));
        mLetters.push_back(Letter('b'));
        mLetters.push_back(Letter('n'));
        mLetters.push_back(Letter('m'));
    }

    Keyboard(const Keyboard &other)
        : mLetters(other.mLetters) { }

    Keyboard updateWithGuess(const std::string &guess, const Match &match) const {
        std::vector<const Letter> updated_keyboard;
        for (auto letter : mLetters) {
            auto ofs = guess.find(letter.value());
            if (ofs == std::string::npos) {
                updated_keyboard.push_back(letter);
            }
            else {
                Match::Value v = match.value_at(ofs);
                updated_keyboard.push_back(Letter(letter, v == Match::kAbsent ? Letter::kAbsent : Letter::kPresent));
            }
        }
        return Keyboard(updated_keyboard);
    }

    const Letter &letter(char c) const {
        auto it = std::find_if(mLetters.begin(), mLetters.end(), [c](const Letter &l) { return l.value() == c; } );
        assert(it != mLetters.end());
        return *it;
    }

    void print() const {
        for (auto letter : mLetters) {
            switch (letter.state()) {
                case Letter::kAbsent:
                    std::cout << "⬛️";
                    break;
                case Letter::kPresent:
                    std::cout << "🟨";
                    break;
                case Letter::kUntested:
                    std::cout << "⬜️";
                    break;
            }
            std::cout << letter.value();
            if (letter.value() == 'p') {
                std::cout << std::endl << "  ";
            } else if (letter.value() == 'l') {
                std::cout << std::endl << "     ";
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }

private:
    Keyboard(const std::vector<const Letter> &letters)
        :mLetters(letters) { }

    std::vector<const Letter> mLetters;
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
