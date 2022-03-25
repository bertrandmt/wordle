// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <cassert>
#include <iostream>

#include "config.h"
#include "state.h"
#include "statecache.h"

StateCache::iterator StateCache::find(const Words &key) {
    std::shared_lock sl(mMutex);

    return mCache.find(key);
}

StateCache::iterator StateCache::end() noexcept {
    std::shared_lock sl(mMutex);

    return mCache.end();
}

std::pair<StateCache::iterator, bool> StateCache::insert(Words &key, State value) {
    std::unique_lock ul(mMutex);

    auto it = mCache.insert(std::make_pair(key, value));
    if (!it.second) {
        assert(it.first->second.words_equal_to(key));
#if DEBUG_STATE_CACHE
        std::cout << "FAILED to insert state with filtered words: " << std::endl;
        std::for_each(key.begin(), key.end(), [](const Word &w) { std::cout << "\"" << w.word() << "\", "; });
        std::cout << std::endl
                  << "It was probably inserted concurrently; continuing" << std::endl;
#endif // DEBUG_STATE_CACHE
    }
    return it;
}

