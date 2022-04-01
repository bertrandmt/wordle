// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <functional>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "keyboard.h"
#include "match.h"
#include "state.h"
#include "statecache.h"
#include "threadpool.h"
#include "wordlist.h"

int main(void) {
    std::cout << Match("clump", "perch").toString() << std::endl;
    std::cout << Match("perch", "clump").toString() << std::endl;
    std::cout << Match("tuner", "exits").toString() << std::endl;
    std::cout << Match("exits", "tuner").toString() << std::endl;
    std::cout << Match("doozy", "yahoo").toString() << std::endl;
    std::cout << Match("preen", "hyper").toString() << std::endl;
    std::cout << Match("hyper", "upper").toString() << std::endl;
    std::cout << Match("ulama", "offal").toString() << std::endl;
    std::cout << Match("tepee", "venom").toString() << std::endl;
    std::cout << Match("venom", "tepee").toString() << std::endl;

    return 0;
}
