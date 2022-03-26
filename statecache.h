// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <memory>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

#include "state.h"

template <>
struct std::hash<Words> {
    std::size_t operator()(Words const& words) const noexcept {
        std::stringstream ss;
        std::for_each(words.begin(), words.end(), [&ss](const Word &s) { ss << s.word(); });
        std::string s = ss.str();
        return std::hash<std::string>{}(s);
    }
};

template <>
struct std::equal_to<Words> {
    bool operator()(const Words &lhs, const Words &rhs) const {
        if (lhs.size() != rhs.size()) return false;
        for (auto lit = lhs.cbegin(), rit = rhs.cbegin(); lit != lhs.end(); lit++, rit++) {
            if (lit->word() != rit->word()) return false;
        }
        return true;
    }
};

class StateCache {
public:
    inline StateCache()
        : mTotalHits(0)
        , mTotalMisses(0)
        , mTotalInserts(0)
        , mHitsSinceLastReport(0)
        , mMissesSinceLastReport(0)
        , mInsertsSinceLastReport(0) { }

    typedef std::unordered_map<Words, std::shared_ptr<State>>::iterator iterator;

    bool contains(const Words &key) const;
    std::shared_ptr<State> at(const Words &key);
    std::pair<iterator, bool> insert(const Words &key, std::shared_ptr<State> value);

    std::string report();

private:
    std::unordered_map<Words, std::shared_ptr<State>> mCache;
    mutable std::shared_mutex mMutex;

    mutable std::size_t mTotalHits;
    mutable std::size_t mTotalMisses;
    mutable std::size_t mTotalInserts;

    mutable std::size_t mHitsSinceLastReport;
    mutable std::size_t mMissesSinceLastReport;
    mutable std::size_t mInsertsSinceLastReport;
};