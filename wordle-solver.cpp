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
#include "keyboard.h"
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

        if (state->n_solutions() == 2) {
            std::cout << ">>>>> SOLUTION ONE OF: " << state->solutions().at(0) << ", " << state->solutions().at(1) << " <<<<<" << std::endl;
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

} // namespace anonymous

class GameStates {
public:
    enum nStates : int {
        kWordleNStates = 1,
        kQuordleNStates = 4,
        kOctordleNStates = 8,
    };

    GameStates(GameState &initial_game_state)
        : mInitialGameState(initial_game_state)
        , mCurrentGame(kWordleNStates)
        , mCurrentGameStates(mWordleStates) {
       reset();
    }

    void reset() {
        for (auto i = 0; i < mCurrentGame; i++) {
            mCurrentGameStates[i].clear();
            mCurrentGameStates[i].push_back(mInitialGameState);
        }
        mInitialGameState.serialize(std::cout);
    }

    void back_one() {
       if (mCurrentGameStates[0].size() == 1) {
           std::cout << "Already at initial state" << std::endl;
           return;
       }

       for (auto i = 0; i < mCurrentGame; i++) {
           mCurrentGameStates[i].pop_back();
           mCurrentGameStates[i].back().serialize(std::cout);
           if (mCurrentGameStates[i].size() != 1) { // not back at initial game state
              mCurrentGameStates[i].back().display_best_guesses();
           }
       }
    }

    void switch_game() {
       switch (mCurrentGame) {
           case kWordleNStates:
               mCurrentGame = kQuordleNStates;
               mCurrentGameStates = mQuordleStates;
               break;
           case kQuordleNStates:
               mCurrentGame = kOctordleNStates;
               mCurrentGameStates = mOctordleStates;
               break;
           case kOctordleNStates:
               mCurrentGame = kWordleNStates;
               mCurrentGameStates = mWordleStates;
               break;
       }
       reset();
    }

    nStates current_game() const { return mCurrentGame; }
    nStates next_game() const {
        switch (mCurrentGame) {
            case kWordleNStates: return kQuordleNStates;
            case kQuordleNStates: return kOctordleNStates;
            case kOctordleNStates: return kWordleNStates;
        }
        assert(!"unreachable");
        return kWordleNStates;
    }

    GameState &at(std::size_t i) const {
        return mCurrentGameStates[i % mCurrentGame].back();
    }

    void process_guess(const std::string &guess, const std::vector<std::string> &matches) {
        if (matches.size() != static_cast<std::size_t>(mCurrentGame)) {
            help();
            return;
        }

        for (auto i = 0; i < mCurrentGame; i++) {
            auto &gs = mCurrentGameStates[i].back();
            if (gs.state->n_solutions() == 1) {
                mCurrentGameStates[i].push_back(gs);
                gs.display_best_guesses();
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

                std::cout << "Considering guess \"" << guess << "\" with match " << m.toString() << std::endl;
                auto s = gs.state->consider_guess(guess, m.value());
                auto k = gs.keyboard.update_with_guess(guess, m);
                GameState gt(gs.generation + 1, s, k);
                gt.serialize(std::cout);
                mCurrentGameStates[i].push_back(gt);
                gt.display_best_guesses();
            }
        }
    }

private:
    GameState const& mInitialGameState;

    nStates mCurrentGame;
    std::vector<GameState> *mCurrentGameStates;

    std::vector<GameState> mWordleStates[kWordleNStates];
    std::vector<GameState> mQuordleStates[kQuordleNStates];
    std::vector<GameState> mOctordleStates[kOctordleNStates];
};

namespace {

bool subroutine(ThreadPool &pool, std::mutex &mutex, std::condition_variable &cond, GameStates &game_states, const StateCache::ptr &state_cache) {
    bool done = false;
    std::string line;
    std::cout << "] " << std::flush;
    std::getline(std::cin, line);
    if (!std::cin) {
        std::cout << std::endl;
        done = true;
        return done;
    }

    // no whitespace we care to make use of
    std::string nowsline(line, 0);
    nowsline.erase(std::remove_if(nowsline.begin(), nowsline.end(), [](auto c){ return std::isspace(c); }), nowsline.end());
    if (nowsline.size() == 0) { return done; }

    switch(nowsline[0]) {
        case '#': // it's a comment
            std::cout << line << std::endl;
            return done;

        case '!': // reset!
            std::cout << "# RESET!" << std::endl;
            game_states.reset();
            return done;

        case '^': // back one
            std::cout << "^ BACK ONE" << std::endl;
            game_states.back_one();
            return done;

        case '%': // change number of concurrent games
            std::cout << "% SWITCHING TO " << game_states.next_game() << " CONCURRENT GAMES" << std::endl;
            game_states.switch_game();
            return done;

        case '*': // persist!
            std::cout << "* PERSISTING CACHE" << std::endl;
            state_cache->persist();
            return done;

        case '?': { // what is the entropy of the word?
            std::string word = nowsline.substr(1);
            for (auto i = 0; i < game_states.current_game(); i++) {
                std::cout << "[" << i << "] H(\"" << word << "\") = "
                          << game_states.at(i).state->entropy_of(word) / 1000. << std::endl;
                std::cout << "[" << i << "]H2(\"" << word << "\") = "
                          << game_states.at(i).state->entropy2_of(word) / 1000. << std::endl;
            }
            }
            return done;

        default:
            break;
    }

    auto ofs = nowsline.find(';');
    std::string guess = nowsline.substr(0, ofs);

    std::vector<std::string> matches;
    while (ofs != std::string::npos) {
        auto ofs2 = nowsline.find(';', ofs + 1);
        std::string match = nowsline.substr(ofs + 1, ofs2 - ofs - 1) ;
        matches.push_back(match);
        ofs = ofs2;
    }
    game_states.process_guess(guess, matches);
#if DEBUG_STATE_CACHE
    std::cout << state_cache->report() << std::endl;
#endif // DEBUG_STATE_CACHE

    return done;
}

void routine(ThreadPool &pool, std::mutex &mutex, std::condition_variable &cond, bool &done, GameStates &game_states, const StateCache::ptr &state_cache) {
    bool subdone = subroutine(pool, mutex, cond, game_states, state_cache);
    if (subdone) {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
    }
    else {
        pool.push([&pool, &mutex, &cond, &done, &game_states, &state_cache]() { routine(pool, mutex, cond, done, game_states, state_cache); });
    }
    cond.notify_all();
}

} // namespace anonymous

int main(void) {
    ThreadPool pool;
    StateCache::ptr state_cache(new StateCache);
    Wordlist word_list;

    State::ptr initial_state(new State(pool, state_cache, word_list.all_words()));
    auto p = state_cache->insert(initial_state);
    assert(p.second);

    auto c = StateCache::restore(state_cache);
    assert(c == state_cache);

    Keyboard initial_keyboard;

    GameState initial_gamestate(1, state_cache->initial_state(), initial_keyboard);
    GameStates game_states(initial_gamestate);

    std::mutex mutex;
    std::condition_variable cond;
    bool done = false;

    pool.push([&pool, &mutex, &cond, &done, &game_states, &state_cache]() { routine(pool, mutex, cond, done, game_states, state_cache); });

    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [&done]() { return done; });
    }

    state_cache->persist();
    pool.done();

    return 0;
}
