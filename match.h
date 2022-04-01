// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <string>
#include <vector>


class Match {
public:
    enum Value : int {
        kAbsent = 0,
        kPresent = 1,
        kCorrect = 2,
    };
    static const size_t kMaxValue = 242;

    Match(const std::string &guess, const std::string &solution);
    Match(const std::string &guess, uint32_t match);

    static Match fromString(const std::string &guess, const std::string match_string, bool &ok);

    std::string toString() const;
    uint32_t value() const;

    inline Value value_at(size_t i) const {
        auto it = mMatch.begin();
        std::advance(it, i);
        if (it == mMatch.end()) {
            return kAbsent;
        }
        return *it;
    }

private:
    std::vector<Value> mMatch;
};
