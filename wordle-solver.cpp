// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "match.h"
#include "state.h"
#include "threadpool.h"
#include "wordlist.h"

void help(void) {
    std::cout << "Enter your successive guesses, along with the outcome in the format:" << std::endl
              << "    guess;cc_pp" << std::endl
              << "  where:" << std::endl
              << "    * \"guess\" is the word you guessed (it must be one of the allowed Wordle words), and," << std::endl
              << "      separated by a colon (';')" << std::endl
              << "    * a five character representation of the outcome where '_' indicates no match, 'p'" << std::endl
              << "      indicates present and 'c' indicates correct." << std::endl;
}

int main(void) {
#if 0
    std::cout << Match("clump", "perch").toString() << std::endl;
    std::cout << Match("perch", "clump").toString() << std::endl;
    std::cout << Match("tuner", "exits").toString() << std::endl;
    std::cout << Match("exits", "tuner").toString() << std::endl;
    std::cout << Match("doozy", "yahoo").toString() << std::endl;
    std::cout << Match("preen", "hyper").toString() << std::endl;
    std::cout << Match("hyper", "upper").toString() << std::endl;
    std::cout << Match("ulama", "offal").toString() << std::endl;
    return 1;
#endif

    ThreadPool pool;

    std::vector<const Word> words;
    for (auto w : solutions) {
        words.push_back(Word(w, true));
    }
    for (auto w : allowed) {
        words.push_back(Word(w, false));
    }

    std::vector<const State> states[N_GAMES];
    for (auto i = 0; i < N_GAMES; i++) {
        states[i].push_back(State(pool, words));
    }

    std::cout << "Initial best guess is \"trace\"." << std::endl;

    for (std::string line; std::cout << "] " << std::flush && std::getline(std::cin, line);) {

        // no whitespace we care to make use of
        std::string nowsline(line, 0);
        nowsline.erase(std::remove_if(nowsline.begin(), nowsline.end(), [](unsigned char c){return std::isspace(c);}), nowsline.end());

        if (nowsline.size() == 0) { // it was all white space
            continue;
        }

        switch(nowsline[0]) {
            case '#': // it's a comment
                std::cout << line << std::endl;
                continue;

            case '!': // reset!
                std::cout << "# RESET!" << std::endl;
                for (auto i = 0; i < N_GAMES; i++) {
                    while (states[i].size() > 1) states[i].pop_back();
                    states[i].back().print();
                }
                continue;

            case '^': // back one
                std::cout << "^ BACK ONE" << std::endl;
                for (auto i = 0; i < N_GAMES; i++) {
                    states[i].pop_back();
                    states[i].back().print();
                    states[i].back().best_guess();
                }
                continue;

            case '?': { // what is the entropy of the word?
                std::string word = nowsline.substr(1);
                for (auto i = 0; i < N_GAMES; i++) {
                    std::cout << "[" << i << "] H(\"" << word << "\") = " << states[i].back().entropy_of(word) << std::endl;
                    std::cout << "[" << i << "]H2(\"" << word << "\") = " << states[i].back().entropy2_of(word) << std::endl;
                }
                }
                continue;

            default:
                break;
        }

        auto ofs = nowsline.find(';');
        std::string guess = nowsline.substr(0, ofs);

        std::vector<std::string> matches;
        while (ofs != std::string::npos) {
            auto ofs2 = nowsline.find(';', ofs+1);
            std::string match = nowsline.substr(ofs + 1, ofs2 - ofs - 1) ;
            matches.push_back(match);
            ofs = ofs2;
        }
        if (matches.size() != N_GAMES) {
            help();
            continue;
        }

        for (auto i = 0; i < N_GAMES; i++) {
            if (states[i].back().n_solutions() == 1) {
                auto s = states[i].back();
                states[i].push_back(s);
                s.best_guess();
            }
            else {
                if (guess.size() != matches[i].size()) {
                    help();
                    continue;
                }

                bool ok = true;
                Match m = Match::fromString(guess, matches[i], ok);
                if (!ok) {
                    help();
                    continue;
                }

                auto s = states[i].back().consider_guess(guess, m.value());
                states[i].push_back(s);
                s.best_guess();
            }
        }
    }

    pool.done();
    return 0;
}
