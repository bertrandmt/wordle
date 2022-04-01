#include "config.h"
#include "keyboard.h"
#include "match.h"
#include "state.h"
#include "statecache.h"
#include "threadpool.h"
#include "wordlist.h"

#include <fstream>

namespace {
void recurse(int level, const std::string &guess, const Keyboard &keyboard, const State::ptr &state, const StateCache::ptr &state_cache) {
    for (std::size_t i = 0; i <= Match::kMaxValue; i++) {
        //if (level == 2) {
        //    auto fw = state->filtered_words_for_guess(guess, i);
        //    if (state_cache->contains(&fw) && state_cache->at(&fw)->is_fully_computed()) continue;
        //}

        Match m(guess, i);
        if (level == 0) {
            for (std::size_t j = 0; j < level; j++) { std::cout << "  "; }
            std::cout << "Considering guess \"" << guess << "\" with match " << m.toString() << std::endl;
        }
        auto t = state->consider_guess(guess, i);

        auto l = keyboard.update_with_guess(guess, m);
        auto e = t->best_guess(l);

        if (t->n_solutions() == 1) continue;
        if (level == 2) continue;

        for (auto &se : e) {
            recurse(level + 1, se.entropy().word().word(), l, t, state_cache);
        }
        if (level == 0 && (i+1)%10 == 0) {
            std::cout << state_cache->report() << std::endl;

            if (state_cache->dirty()) {
                std::cout << "] " << std::flush;

                std::string line;
                std::getline(std::cin, line);
            }

            state_cache->persist();
        }
    }
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

    recurse(0, "trace", initial_keyboard, initial_state, state_cache);

    state_cache->persist();
    pool.done();

    return 0;
}
