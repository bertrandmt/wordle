#include <cassert>
#include <iostream>

#include "config.h"
#include "match.h"

namespace {

std::string kMatchValueString[] = {
    "⬜️",
    "🟨",
    "🟩"
};

} // namespace anonymous

Match::Match(const std::string &guess, const std::string &solution)
    : mMatch(guess.size(), kAbsent) {

    assert(guess.size() == solution.size());
#if DEBUG_MATCH
    std::cout << "\"" << guess << "\" | \"" << solution << "\"" << std::endl;
#endif

    std::vector<Value> solution_match(solution.size(), kAbsent);
    // pass 1: considering each guess letter for correctness
    for (auto i = 0; i < guess.size(); i++) {
#if DEBUG_MATCH
        std::cout << "Considering guess[" << i << "]('" << guess[i] << "') for " << kMatchValueString[kCorrect] << ": " << std::flush;
#endif
        if (guess[i] == solution[i]) {
            mMatch[i] = kCorrect;
            solution_match[i] = kCorrect;
        }
#if DEBUG_MATCH
        std::cout << kMatchValueString[mMatch[i]] << std::endl;
#endif
    }

    // pass 2: considering each guess letter for presence
    for (auto i = 0; i < guess.size(); i++) {
#if DEBUG_MATCH
        std::cout << "Considering guess[" << i << "]('" << guess[i] << "') for " << kMatchValueString[kPresent] << ":" << std::endl;
#endif

        if (mMatch[i] == kCorrect) {
#if DEBUG_MATCH
            std::cout << "  already determined " << kMatchValueString[kCorrect] << ". Continuing." << std::endl;
#endif
            continue;
        }

        bool present = false;

        for (auto j = 0; j < solution.size(); j++) {
#if DEBUG_MATCH
            std::cout << "  comparing to solution[" << j << "]('" << solution[j] << "'): " << std::flush;
#endif

            if (solution_match[j] != kAbsent) {
#if DEBUG_MATCH
                std::cout << "previously \"consumed\" as " << kMatchValueString[solution_match[j]] << "; skipping" << std::endl;
#endif
                continue;
            }

            if (guess[i] != solution[j]) {
#if DEBUG_MATCH
                std::cout << "no match." << std::endl;
#endif
                continue;
            }

            //if (guess[i] == solution[j]) {
            assert(i != j); // otherwise solution_match[j] would have been marked as kCorrect above
#if DEBUG_MATCH
            std::cout << kMatchValueString[kPresent] << std::endl;
#endif
            solution_match[j] = kPresent;
            present = true;
            break;
        }
        if (present) {
            assert(mMatch[i] != kCorrect);
            mMatch[i] = kPresent;
        }
#if DEBUG_MATCH
        std::cout << "  outcome: " << kMatchValueString[mMatch[i]] << std::endl;
#endif
    }
}

Match::Match(const std::string &guess, uint32_t match)
    : mMatch(guess.size()) {

    for (auto i = 0; i < mMatch.size(); i++) {
        mMatch[i] = static_cast<Value>(match % 3);
        match /= 3;
    }
}

Match Match::fromString(const std::string &guess, const std::string match_string, bool &ok) {
    uint32_t match_value = 0;
    uint32_t exponent = 1;
    ok = true;
    for (auto i = 0; i < match_string.size(); i++) {
        switch (match_string[i]) {
            case 'c':
            case 'C':
                match_value += 2 * exponent;
                break;
            case 'p':
            case 'P':
                match_value += exponent;
                break;
            case '_':
            case '-':
            case 'a':
                break;
            default:
                ok = false;
                break;
        }
        exponent *= 3;
        if (!ok) break;
    }
    return Match(guess, match_value);
}

std::string Match::toString() const {
    std::string s;
    for (auto i = 0; i < mMatch.size(); i++) {
        switch (mMatch[i]) {
            case kCorrect:
                s.append(reinterpret_cast<const char *>(u8"🟩"));
                break;
            case kPresent:
                s.append(reinterpret_cast<const char *>(u8"🟨"));
                break;
            case kAbsent:
                s.append(reinterpret_cast<const char *>(u8"⬜️"));
                break;
            default:
                s.append(reinterpret_cast<const char *>(u8"💀"));
                break;
        }
    }
    return s;
}

uint32_t Match::value() const {
    uint32_t value = 0;
    uint32_t exponent = 1;
    for (auto i = 0; i < mMatch.size(); i++) {
        value += mMatch[i] * exponent;
        exponent *= 3;
    }
    return value;
}
