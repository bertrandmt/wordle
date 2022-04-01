// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#ifndef WORD_LIST_H
#define WORD_LIST_H

#include <vector>
#include <string>

#include "word.h"

class Wordlist {
public:
    Wordlist();
    const Words &all_words() const { return mAllWords; }

private:
    const Words mAllWords;
};

#endif // WORD_LIST_H
