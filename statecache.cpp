// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <cassert>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>

#include "config.h"
#include "state.h"
#include "statecache.h"

bool StateCache::contains(const Words *key) const {
    std::shared_lock sl(mMutex);

    return mCache.contains(key);
}

State::ptr StateCache::at(const Words *key) const {
    std::shared_lock sl(mMutex);

    auto s = mCache.at(key);
    mTotalHits++;
    mHitsSinceLastReport++;
    return s;
}

std::pair<StateCache::iterator, bool> StateCache::insert(State::ptr value) {
    std::unique_lock ul(mMutex);

    mTotalMisses++;
    mMissesSinceLastReport++;

    auto key = value->words_ptr();
    auto it = mCache.insert(std::make_pair(key, value));
    if (!it.second) {
        assert(it.first->second->words_equal_to(*key));
#if DEBUG_STATE_CACHE
        std::cout << "FAILED to insert state with filtered words: " << std::endl;
        std::for_each(key->begin(), key->end(), [](const Word &w) { std::cout << "\"" << w.word() << "\", "; });
        std::cout << std::endl
                  << "It was probably inserted concurrently; continuing" << std::endl;
#endif // DEBUG_STATE_CACHE
    }
    else {
        mDirty = true;

        if (!mInitialState.get()) {
            mInitialState = value;
        }

        mTotalInserts++;
        mInsertsSinceLastReport++;
    }
    return it;
}

std::string StateCache::report() {
    std::size_t total_events = mTotalHits + mTotalMisses;
    std::size_t events_since_last_report = mHitsSinceLastReport + mMissesSinceLastReport;

    std::size_t n_fully_computed = std::transform_reduce(mCache.begin(), mCache.end(), 0, std::plus<>(), [](auto &entry) -> std::size_t { if (entry.second->is_fully_computed()) return 1; else return 0; });
    std::size_t total_entropy_entries = std::transform_reduce(mCache.begin(), mCache.end(), 0, std::plus<>(), [](auto &entry) -> std::size_t { return entry.second->n_entropies(); });

    std::stringstream ss;
    ss << "E:" << mCache.size() << "(F:" << n_fully_computed << ")(avg " << (total_entropy_entries * 1.) / mCache.size() << " h/s)" << std::endl
       << "T:H:" << mTotalHits           << "|M:" << mTotalMisses           << "|I:" << mTotalInserts           << " / " << total_events << std::endl
       << "S:H:" << mHitsSinceLastReport << "|M:" << mMissesSinceLastReport << "|I:" << mInsertsSinceLastReport << " / " << events_since_last_report;

    mHitsSinceLastReport = 0;
    mMissesSinceLastReport = 0;
    mInsertsSinceLastReport = 0;

    return ss.str();
}

void StateCache::serialize(std::ostream &os) const {
    assert(mCache.size() <= std::numeric_limits<uint32_t>::max());
    uint32_t sz = mCache.size() - 1;
    os.write(reinterpret_cast<char *>(&sz), sizeof sz);

    std::for_each(mCache.begin(), mCache.end(), [&os, this](const auto &cache_entry) {
            if (cache_entry.second == mInitialState) { // skip initial state
                return;
            }
            cache_entry.second->serialize(os);
        });

    mDirty = false;
}

void StateCache::persist() const {
    if (!mDirty) return;

    std::cout << "Persisting state cache..." << std::flush;

    std::ofstream ofs;
    ofs.open("wordle_state_cache.bin", std::ofstream::trunc|std::ofstream::binary);

    serialize(ofs);

    ofs.close();
    std::cout << " done" << std::endl;
}

StateCache::ptr StateCache::unserialize(StateCache::ptr &init, std::istream &is) {
    uint32_t n_states;
    is.read(reinterpret_cast<char *>(&n_states), sizeof n_states);

    for (size_t i = 0; i < n_states; i++) {
        State::ptr state = State::unserialize(is, init);
        auto p = init->insert(state);
        assert(p.second);
    }

    init->mDirty = false;

    return init;
}

StateCache::ptr StateCache::restore(StateCache::ptr &init) {
    std::cout << "Loading state cache..." << std::flush;

    std::ifstream ifs;
    ifs.open("wordle_state_cache.bin", std::ifstream::binary);
    if (ifs.fail()) {
        std::cout << " failed: initializing from scratch" << std::endl;
        return init;
    }

    auto c = StateCache::unserialize(init, ifs);
    assert(c == init);

    ifs.close();
    std::cout << " done" << std::endl;

    init->reset_stats();
    std::cout << init->report() << std::endl;

    return init;
}
