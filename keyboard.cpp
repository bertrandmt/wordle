// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include "config.h"
#include "keyboard.h"
#include "match.h"

#include <algorithm>
#include <cassert>
#include <iostream>

namespace {
std::vector<Letter> initial_keyboard() {
    std::vector<Letter> letters;

    letters.reserve(26);
    letters.push_back(Letter('q'));
    letters.push_back(Letter('w'));
    letters.push_back(Letter('e'));
    letters.push_back(Letter('r'));
    letters.push_back(Letter('t'));
    letters.push_back(Letter('y'));
    letters.push_back(Letter('u'));
    letters.push_back(Letter('i'));
    letters.push_back(Letter('o'));
    letters.push_back(Letter('p'));
    letters.push_back(Letter('a'));
    letters.push_back(Letter('s'));
    letters.push_back(Letter('d'));
    letters.push_back(Letter('f'));
    letters.push_back(Letter('g'));
    letters.push_back(Letter('h'));
    letters.push_back(Letter('j'));
    letters.push_back(Letter('k'));
    letters.push_back(Letter('l'));
    letters.push_back(Letter('z'));
    letters.push_back(Letter('x'));
    letters.push_back(Letter('c'));
    letters.push_back(Letter('v'));
    letters.push_back(Letter('b'));
    letters.push_back(Letter('n'));
    letters.push_back(Letter('m'));

    return letters;
}
}

Keyboard::Keyboard()
    : mLetters(initial_keyboard()) { }

Keyboard Keyboard::update_with_guess(const std::string &guess, const Match &match) const {
    std::vector<Letter> updated_keyboard;
    updated_keyboard.reserve(mLetters.size());

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


