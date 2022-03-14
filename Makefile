CPPFLAGS=-std=c++20 -g -Wall
CC=$(CXX)

wordle: match.o state.o threadpool.o wordlist.o

match.o: match.h config.h
state.o: state.h config.h match.h threadpool.h
threadpool.o: threadpool.h config.h
wordlist.o: wordlist.h config.h
wordle.o: config.h match.h state.h threadpool.h wordlist.h

.PHONY: clean

clean:
	rm -f match.o state.o threadpool.o wordlist.o wordle.o wordle
