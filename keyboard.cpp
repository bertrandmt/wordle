// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include "config.h"
#include "keyboard.h"
#include "match.h"

#include <iostream>

Keyboard::Keyboard() {
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

Keyboard Keyboard::updateWithGuess(const std::string &guess, const Match &match) const {
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

const Letter &Keyboard::letter(char c) const {
    auto it = std::find_if(mLetters.begin(), mLetters.end(), [c](const Letter &l) { return l.value() == c; } );
    assert(it != mLetters.end());
    return *it;
}

void Keyboard::print() const {
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


