// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <limits>
#include <cstdint>

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
        os.put(mIsSolution);

        assert(mWord.size() <= std::numeric_limits<char>::max());
        os.put(static_cast<char>(mWord.size()));

        os.write(&mWord[0], mWord.size());
    }

    static inline Word unserialize(std::istream &is) {
        char is_solution_c;
        is.get(is_solution_c);
        bool is_solution = static_cast<bool>(is_solution_c);

        char word_len_c;
        is.get(word_len_c);
        size_t word_len = word_len_c;

        if (word_len != WORD_LEN) {
            throw new std::runtime_error("bad string size value");
        }

        char w[word_len];
        is.read(w, word_len);
        std::string word(w, word_len);

        return Word(word, is_solution);
    }

    bool operator ==(const Word &other) const {
        return mWord == other.mWord
            && mIsSolution == other.mIsSolution;
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

        os.write(reinterpret_cast<const char *>(&mEntropy), sizeof mEntropy);
    }

    static inline WordEntropy unserialize(std::istream &is) {
        Word word = Word::unserialize(is);

        uint32_t entropy;
        is.read(reinterpret_cast<char *>(&entropy), sizeof entropy);

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


