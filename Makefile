CPPFLAGS=-std=c++20 -O3 # -g -Wall
CC=$(CXX)

wordle-solver: keyboard.o match.o state.o statecache.o threadpool.o wordlist.o

keyboard.o: keyboard.h match.h config.h
match.o: match.h config.h
state.o: state.h config.h keyboard.h match.h threadpool.h
statecache.o: statecache.h state.h config.h
threadpool.o: threadpool.h config.h
wordlist.o: wordlist.h config.h
wordle-solver.o: config.h match.h state.h statecache.h threadpool.h wordlist.h

.PHONY: clean

clean:
	rm -f match.o state.o threadpool.o wordlist.o wordle-solver.o wordle-solver
