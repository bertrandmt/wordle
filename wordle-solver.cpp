// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <functional>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "match.h"
#include "state.h"
#include "statecache.h"
#include "threadpool.h"
#include "wordlist.h"

namespace {

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

} // namespace anonymous

struct GameState {
    GameState(int g, const State::ptr &s, const Keyboard &k)
        : generation(g)
        , state(s)
        , keyboard(k) { }

    inline void serialize(std::ostream &os) const {
        os << "State[gen:" << generation << "]: S:" << state->n_solutions() << "|W:" << state->n_words() << std::endl;
        if (generation == 1) {
            os << "Initial best guess is \"trace\"." << std::endl;
        }
    }

    void display_best_guesses() {
        if (state->n_solutions() == 1) {
            std::cout << ">>>>> THE SOLUTION: \"" << state->solutions().at(0).word() << "\" <<<<<" << std::endl;
            return;
        }

        auto best_guesses = state->best_guess(keyboard);
        if (best_guesses.size() == 0) {
            std::cout << "No solution left 😭" << std::endl;
            return;
        }

        if (state->n_solutions() <= MAX_N_SOLUTIONS_PRINTED) {
            std::cout << "Solutions and associated entropy: ";
            bool first = true;
            for (auto entropy : state->solution_entropies()) {
                if (first) first = false;
                else       std::cout << ", ";
                std::cout << entropy;
            }
            std::cout << std::endl;
        }

        std::cout << "[H=" << best_guesses.front().entropy().entropy() / 1000. << "|S=" << best_guesses.front().score()
                  << "] \"" << select_randomly(best_guesses.begin(), best_guesses.end())->entropy().word().word() << "\"";
        if (best_guesses.size() > 1) {
            std::cout << " (" << best_guesses.size() << " words: ";
            bool first = true;
            for (auto it = best_guesses.begin(); it != best_guesses.end() && distance(best_guesses.begin(), it) < MAX_N_GUESSES_PRINTED; it++) {
                if (!first) std::cout << ", ";
                first = false;
                std::cout << "\"" << it->entropy().word().word() << "\"";
            }
            if (best_guesses.size() > MAX_N_GUESSES_PRINTED) {
                std::cout << ", ...";
            }
            std::cout << ") ";
        }
        std::cout << std::endl;
    }

    const int generation;
    const State::ptr state;
    const Keyboard keyboard;
};

namespace {

void help(void) {
    std::cout << "Enter your successive guesses, along with the outcome in the format:" << std::endl
              << "    guess;cc_pp" << std::endl
              << "  where:" << std::endl
              << "    * \"guess\" is the word you guessed (it must be one of the allowed Wordle words), and," << std::endl
              << "      separated by a colon (';')" << std::endl
              << "    * a five character representation of the outcome where '_' indicates no match, 'p'" << std::endl
              << "      indicates present and 'c' indicates correct." << std::endl;
}

}

int main(void) {
    ThreadPool pool;
    StateCache::ptr state_cache(new StateCache);

    Words all_words;
    for (auto w : solutions) {
        all_words.push_back(Word(w, true));
    }
    for (auto w : allowed) {
        all_words.push_back(Word(w, false));
    }

    State::ptr initial_state(new State(pool, state_cache, all_words));
    auto p = state_cache->insert(all_words, initial_state);
    assert(p.second);

    std::cout << "Loading state cache..." << std::flush;
    std::ifstream ifs;
    ifs.open("wordle_state_cache.txt");
    if (ifs.fail()) {
        std::cout << " failed: initializing from scratch" << std::endl;
    }
    else {
        auto c = StateCache::unserialize(state_cache, ifs);
        assert(c == state_cache);
        ifs.close();
        std::cout << " done" << std::endl;

        state_cache->reset_stats();
        std::cout << state_cache->report() << std::endl;
    }
    Keyboard initial_keyboard;

    GameState initial_gamestate(1, state_cache->initial_state(), initial_keyboard);

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
    initial_gamestate.serialize(std::cout);

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
                initial_gamestate.serialize(std::cout);
                continue;

            case '^': // back one
                std::cout << "^ BACK ONE" << std::endl;
                if (current_game_states[0].size() == 1) {
                    std::cout << "Already at initial state" << std::endl;
                    continue;
                }

                for (auto i = 0; i < current_game; i++) {
                    current_game_states[i].pop_back();
                    current_game_states[i].back().serialize(std::cout);
                    if (current_game_states[i].size() != 1) { // not back at initial game state
                       current_game_states[i].back().display_best_guesses();
                    }
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
                initial_gamestate.serialize(std::cout);
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
                s.display_best_guesses();
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
                std::cout << "Considering guess \"" << guess << "\" with match " << m.toString() << std::endl;
                auto s = p.state->consider_guess(guess, m.value());
                auto k = p.keyboard.updateWithGuess(guess, m);
                GameState gs(p.generation + 1, s, k);
                gs.serialize(std::cout);
                current_game_states[i].push_back(gs);
                gs.display_best_guesses();
            }
        }

        std::cout << state_cache->report() << std::endl;
    }

    std::cout << "Persisting state cache..." << std::flush;
    std::ofstream ofs;
    ofs.open("wordle_state_cache.txt", std::ofstream::trunc);
    state_cache->serialize(ofs);
    ofs.close();
    std::cout << " done" << std::endl;

    pool.done();

    return 0;
}


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
#if 0
    {
        std::string input("1 5cigar");
        std::istringstream is(input);
        Word w = Word::unserialize(is);
        w.serialize(std::cout);
        std::cout << std::endl;
    }
    {
        std::string input("1 5cigar 693");
        std::istringstream is(input);
        WordEntropy we = WordEntropy::unserialize(is);
        we.serialize(std::cout);
        std::cout << std::endl;
    }
    return 1;
#endif

#if 0
    {
        std::string input("0 4 1 5actor 0 5carts 0 5curat 0 5scrat 4 1 5actor 0 0 5carts 0 0 5curat 0 0 5scrat 0 ");
        std::istringstream is(input);
        State::ptr state = State::unserialize(is, initial_state);
        state->serialize(std::cout);
    }
    pool.done();
    return 1;
#endif
