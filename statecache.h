// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <memory>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

class State;

template <>
struct std::hash<const Words *> {
    std::size_t operator()(Words const *words) const noexcept {
        std::stringstream ss;
        std::for_each(words->begin(), words->end(), [&ss](const Word &s) { ss << s.word(); });
        std::string s = ss.str();
        return std::hash<std::string>{}(s);
    }
};

template <>
struct std::equal_to<const Words *> {
    bool operator()(const Words *lhs, const Words *rhs) const {
        if (lhs->size() != rhs->size()) { return false; }
        for (auto lit = lhs->cbegin(), rit = rhs->cbegin(); lit != lhs->end(); lit++, rit++) {
            if (lit->word() != rit->word()) { return false; }
        }
        return true;
    }
};

class StateCache {
public:
    typedef std::shared_ptr<StateCache> ptr;
    typedef std::unordered_map<const Words *, std::shared_ptr<State>> map;
    typedef map::iterator iterator;

    inline StateCache()
        : mTotalHits(0)
        , mTotalMisses(0)
        , mTotalInserts(0)
        , mHitsSinceLastReport(0)
        , mMissesSinceLastReport(0)
        , mInsertsSinceLastReport(0) { }
    static ptr unserialize(ptr &init, std::istream &is);
    static ptr restore(ptr &init);

    bool contains(const Words *key) const;
    std::shared_ptr<State> at(const Words *key);
    std::pair<iterator, bool> insert(std::shared_ptr<State> value);

    std::shared_ptr<State> initial_state() const { return mInitialState; }

    inline void reset_stats() {
        mTotalHits = 0;
        mTotalMisses = 0;
        mTotalInserts = 0;
        mHitsSinceLastReport = 0;
        mMissesSinceLastReport = 0;
        mInsertsSinceLastReport = 0;
    }
    std::string report();

    void persist() const;
    void serialize(std::ostream &os) const;

private:
    inline void set_cache(const map &cache) {
        mCache = cache;
    }

    map mCache;
    mutable std::shared_mutex mMutex;
    std::shared_ptr<State> mInitialState;

    mutable std::size_t mTotalHits;
    mutable std::size_t mTotalMisses;
    mutable std::size_t mTotalInserts;

    mutable std::size_t mHitsSinceLastReport;
    mutable std::size_t mMissesSinceLastReport;
    mutable std::size_t mInsertsSinceLastReport;
};
