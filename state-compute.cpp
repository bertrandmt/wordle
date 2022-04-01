#include "config.h"
#include "keyboard.h"
#include "match.h"
#include "state.h"
#include "statecache.h"
#include "threadpool.h"
#include "wordlist.h"

#include <fstream>

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

    for (std::size_t i = 0; i <= Match::kMaxValue; i++) {
        Match m("trace", i);
        std::cout << "Considering guess \"trace\" with match " << m.toString() << std::endl;
        auto s = initial_state->consider_guess("trace", i);

        if (s->n_solutions() == 1) continue;

        auto k = initial_keyboard.update_with_guess("trace", m);
        auto e = s->best_guess(k);

        for (auto se : e) {
            auto guess = se.entropy().word().word();
            for (std::size_t j = 0; j <= Match::kMaxValue; j++) {
                auto fw = s->filtered_words_for_guess(guess, j);
                if (state_cache->contains(&fw) && state_cache->at(&fw)->is_fully_computed()) continue;

                Match n(guess, j);
                std::cout << "    Considering guess \"" << guess << "\" with match " << n.toString() << std::endl;
                auto t = s->consider_guess(guess, j);

                if (t->n_solutions() == 1) continue;
                auto l = k.update_with_guess(guess, n);
                auto f = t->best_guess(l);

#if 0
                if (f.size() == 0) continue;
#endif
            }
        }
        if ((i+1)%10 == 0) {
            std::cout << state_cache->report() << std::endl;

            std::cout << "] " << std::flush;

            std::string line;
            std::getline(std::cin, line);

            state_cache->persist();
        }
    }

    state_cache->persist();
    pool.done();

    return 0;
}
