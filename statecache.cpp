// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include "config.h"
#include "state.h"
#include "statecache.h"

bool StateCache::contains(const Words &key) const {
    std::shared_lock sl(mMutex);

    return mCache.contains(key);
}

std::shared_ptr<State> StateCache::at(const Words &key) {
    std::shared_lock sl(mMutex);

    auto s = mCache.at(key);
    mTotalHits++;
    mHitsSinceLastReport++;
    return s;
}

std::pair<StateCache::iterator, bool> StateCache::insert(const Words &key, std::shared_ptr<State> value) {
    std::unique_lock ul(mMutex);

    mTotalMisses++;
    mMissesSinceLastReport++;

    auto it = mCache.insert(std::make_pair(key, value));
    if (!it.second) {
        assert(it.first->second->words_equal_to(key));
#if DEBUG_STATE_CACHE
        std::cout << "FAILED to insert state with filtered words: " << std::endl;
        std::for_each(key.begin(), key.end(), [](const Word &w) { std::cout << "\"" << w.word() << "\", "; });
        std::cout << std::endl
                  << "It was probably inserted concurrently; continuing" << std::endl;
#endif // DEBUG_STATE_CACHE
    }
    else {
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

    std::stringstream ss;
    ss << "T:H:" << mTotalHits           << "|M:" << mTotalMisses           << "|I:" << mTotalInserts           << " / " << total_events << std::endl
       << "S:H:" << mHitsSinceLastReport << "|M:" << mMissesSinceLastReport << "|I:" << mInsertsSinceLastReport << " / " << events_since_last_report;

    mHitsSinceLastReport = 0;
    mMissesSinceLastReport = 0;
    mInsertsSinceLastReport = 0;

    return ss.str();
}

void StateCache::serialize(std::ostream &os) const {
    os << mCache.size() << " ";
    std::for_each(mCache.begin(), mCache.end(), [&os](const auto &cache_entry) { cache_entry.second->serialize(os); });
}

StateCache::ptr StateCache::unserialize(StateCache::ptr &cache, std::istream &is, ThreadPool &pool, const Words &all_words) {
    std::size_t n_states;
    is >> n_states;

    std::shared_ptr<State> initial_state = State::unserialize(is, pool, cache, all_words);
    auto p = cache->insert(initial_state->words(), initial_state);
    assert(p.second);

    for (size_t i = 1; i < n_states; i++) {
        std::shared_ptr<State> state = State::unserialize(is, pool, cache, all_words);
        p = cache->insert(state->words(), state);
        assert(p.second);
    }

    return cache;
}
