// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <iostream>
#include <string>

class Keyboard;

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

    inline void serialize(std::ostream &os) const {
        os << mIsSolution << " " << mWord.size() << mWord ;
    }

    static inline Word unserialize(std::istream &is) {
        bool is_solution;
        std::size_t word_len;

        is >> is_solution >> word_len;

        if (word_len != 5) {
            throw new std::runtime_error("bad string size value");
        }

        char w[word_len];
        is.read(w, word_len);
        std::string word(w, word_len);

        return Word(word, is_solution);
    }

private:
    std::string mWord;
    bool mIsSolution;
};

std::ostream& operator<<(std::ostream& out, const Word& word);

typedef std::vector<Word> Words;

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

    inline bool operator<(const WordEntropy &other) const {
        return other.mEntropy < mEntropy; // inverted, to achieve decreasing order
    }

    inline void serialize(std::ostream &os) const {
        mWord.serialize(os);
        os << " " << mEntropy;
    }

    static inline WordEntropy unserialize(std::istream &is) {
        Word word = Word::unserialize(is);

        uint32_t entropy;
        is >> entropy;

        return WordEntropy(word, entropy);
    }
private:
    Word mWord;
    uint32_t mEntropy;
};

std::ostream& operator<<(std::ostream& out, const WordEntropy& word_entropy);

class ScoredEntropy {
public:
    ScoredEntropy(const WordEntropy &entropy, const Keyboard &keyboard);
    inline ScoredEntropy(const WordEntropy &entropy, int score = 0)
        : mEntropy(entropy)
	, mScore(score) { }

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

std::ostream& operator<<(std::ostream& out, const ScoredEntropy& scored_entropy);


