#pragma once

#include "match.h"

#include <vector>

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
    Keyboard();
    inline Keyboard(const Keyboard &other)
        : mLetters(other.mLetters) { }

    Keyboard updateWithGuess(const std::string &guess, const Match &match) const;
    const Letter &letter(char c) const;
    void print() const;

private:
    inline Keyboard(const std::vector<const Letter> &letters)
        :mLetters(letters) { }

    std::vector<const Letter> mLetters;
};
