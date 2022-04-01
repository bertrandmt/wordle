CPPFLAGS=-std=c++20 -O3 # -g -Wall
CC=$(CXX)

src = keyboard.cpp match.cpp state.cpp statecache.cpp threadpool.cpp wordlist.cpp

wordle-solver: $(src:%.cpp=%.o)


.PHONY: depend
depend:
	makedepend -- $(CPPFLAGS) -- wordle-solver.cpp $(src)

.PHONY: clean
clean:
	rm -f match.o state.o threadpool.o wordlist.o wordle-solver.o wordle-solver

# DO NOT DELETE

wordle-solver.o: config.h keyboard.h match.h state.h word.h statecache.h
wordle-solver.o: threadpool.h wordlist.h
keyboard.o: config.h keyboard.h match.h
match.o: config.h match.h
state.o: config.h keyboard.h match.h state.h word.h statecache.h threadpool.h
statecache.o: config.h state.h word.h statecache.h
threadpool.o: config.h threadpool.h
wordlist.o: config.h wordlist.h word.h
