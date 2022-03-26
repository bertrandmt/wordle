// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "match.h"
#include "state.h"
#include "statecache.h"
#include "threadpool.h"
#include "wordlist.h"

struct GameState {
    GameState(int g, const std::shared_ptr<State> &s, const Keyboard &k)
        : generation(g)
        , state(s)
        , keyboard(k) { }

    std::string toString() {
        std::stringstream ss;
        ss << "State[gen:" << generation << "]: S:" << state->n_solutions() << "|W:" << state->n_words() ;
        return ss.str();
    }

    const int generation;
    const std::shared_ptr<State> state;
    const Keyboard keyboard;
};

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
    StateCache state_cache;

    std::vector<const Word> words;
    for (auto w : solutions) {
        words.push_back(Word(w, true));
    }
    for (auto w : allowed) {
        words.push_back(Word(w, false));
    }

    std::shared_ptr<State> initial_state(new State(pool, state_cache, words));
    auto p = state_cache.insert(words, initial_state);
    assert(p.second);

    Keyboard initial_keyboard;

    GameState initial_gamestate(1, initial_state, initial_keyboard);

    enum nStates : int {
        kWordleNStates = 1,
        kQuordleNStates = 4,
        kOctordleNStates = 8,
    };


    std::vector<GameState> wordle_states[kWordleNStates];
    std::vector<GameState> quordle_states[kQuordleNStates];
    std::vector<GameState> octordle_states[kOctordleNStates];

    nStates current_game = kWordleNStates;
    std::vector<GameState> *current_game_states = wordle_states;
    for (auto i = 0; i < current_game; i++) {
        current_game_states[i].push_back(initial_gamestate);
    }
    std::cout << initial_gamestate.toString() << std::endl;
    initial_gamestate.state->best_guess(initial_gamestate.generation, initial_gamestate.keyboard);

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
                for (auto i = 0; i < current_game; i++) {
                    current_game_states[i].clear();
                    current_game_states[i].push_back(initial_gamestate);
                }
                std::cout << initial_gamestate.toString() << std::endl;
                initial_gamestate.state->best_guess(initial_gamestate.generation, initial_gamestate.keyboard);
                continue;

            case '^': // back one
                std::cout << "^ BACK ONE" << std::endl;
                if (current_game_states[0].size() == 1) {
                    std::cout << "Already at initial state" << std::endl;
                    continue;
                }

                for (auto i = 0; i < current_game; i++) {
                    current_game_states[i].pop_back();
                    std::cout << current_game_states[i].back().toString();
                    current_game_states[i].back().state->best_guess(current_game_states[i].back().generation, current_game_states[i].back().keyboard);
                }
                continue;

            case '%': // change number of concurrent games
                switch (current_game) {
                    case kWordleNStates:
                        current_game = kQuordleNStates;
                        current_game_states = quordle_states;
                        break;
                    case kQuordleNStates:
                        current_game = kOctordleNStates;
                        current_game_states = octordle_states;
                        break;
                    case kOctordleNStates:
                        current_game = kWordleNStates;
                        current_game_states = wordle_states;
                        break;
                }
                std::cout << "% SWITCHING TO " << current_game << " CONCURRENT GAMES" << std::endl;
                for (auto i = 0; i < current_game; i++) {
                    current_game_states[i].clear();
                    current_game_states[i].push_back(initial_gamestate);
                }
                std::cout << initial_gamestate.toString() << std::endl;
                initial_gamestate.state->best_guess(initial_gamestate.generation, initial_gamestate.keyboard);
                continue;

            case '?': { // what is the entropy of the word?
                std::string word = nowsline.substr(1);
                for (auto i = 0; i < current_game; i++) {
                    std::cout << "[" << i << "] H(\"" << word << "\") = " << current_game_states[i].back().state->entropy_of(word) << std::endl;
                    std::cout << "[" << i << "]H2(\"" << word << "\") = " << current_game_states[i].back().state->entropy2_of(word) << std::endl;
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
        if (matches.size() != current_game) {
            help();
            continue;
        }

        for (auto i = 0; i < current_game; i++) {
            if (current_game_states[i].back().state->n_solutions() == 1) {
                auto s = current_game_states[i].back();
                current_game_states[i].push_back(s);
                s.state->best_guess(s.generation, s.keyboard);
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

                auto p = current_game_states[i].back();
                auto s = p.state->consider_guess(guess, m.value());
                auto k = p.keyboard.updateWithGuess(guess, m);
                GameState gs(p.generation + 1, s, k);
                std::cout << gs.toString() << std::endl;
                current_game_states[i].push_back(gs);
                s->best_guess(gs.generation, k);
            }
        }

        std::cout << state_cache.report() << std::endl;
    }

    pool.done();
    return 0;
}
