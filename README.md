# wordle-solver
`wordle-solver` is a solving tool for [Wordle](https://www.nytimes.com/games/wordle/index.html).

# Usage

```
$ ./wordle-solver
State[gen#1]: 2315 solutions and 12960 words.
⬜️q ⬜️w ⬜️e ⬜️r ⬜️t ⬜️y ⬜️u ⬜️i ⬜️o ⬜️p
  ⬜️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Initial best guess is "trace".
] 
```

At the prompt (`]`), enter a guess and its outcome as reported by Wordle. The solver recommends `trace` as the initial guess, though the user is free to choose any starting word it cares.

For Wordle #268 (Monday 2022-03-14), let's try `aiery` as the initial guess. World reports the `E` being 🟩 with all other letters (`A`, `I`, `R` and `Y`) being ⬜️.

The syntax to tell as much to `wordle-solver` is in the form `guess;match`, with match a series of letters indicating the outcome.

⬜️ can be represented by `a` or `A` (as in "absent"), or `_` or `-`.

🟨 can be represented by `p` or `P` (as in "present").

🟩 can be represented by `c` or `C` (as in "correct").

For our example, we'd enter `aiery;aacaa`. `world-solver` immediately proceeds to analyze the outcome and propose a "most useful" next guess, according to a few heuristics.

```
] aiery;aacaa
Considering guess "aiery" with match ⬜️⬜️🟩⬜️⬜️
State[gen#2]: 64 solutions and 352 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬜️u ⬛️i ⬜️o ⬜️p
  ⬛️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Computing entropy...................................
Recommending guess "slump" out of 1 words ("slump") with highest two-level entropy 4.13722 and highest score 15.
] 
```

Again, the user is not required to make use of the recommendation (though, then, what is the point of using `wordle-solver`?).

As is reported, the space of solutions at the 2nd generation has shrunk from 12960 words and 2315 possible solutions, down to 352 words and only 64 possible solutions.

As further entries are provided, the space of solutions goes down until there are few enough to print all of them, or there is just one.

```
] slump;cpapa
Considering guess "slump" with match 🟩🟨⬜️🟨⬜️
State[gen#3]: 2 solutions and 3 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬛️u ⬛️i ⬜️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
Solutions and associated entropy:
	"smelt": 0.693147
	"smell": 0
Recommending guess "wootz" out of 4 words ("wootz", "whoot", "tondo", "thoft") with highest two-level entropy 0.693147 and highest score 15.
] smell;cccca
Considering guess "smell" with match 🟩🟩🟩🟩⬜️
State[gen#4]: 1 solutions and 1 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬛️u ⬛️i ⬜️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
THE solution: "smelt"
] 
```

The keen eye will notice that the recommendation at the 3rd generation is not useful in this example. `wordle-solver` is not perfect yet!

Ultimately, there is only one solution left (`smelt`) and the solver indicates as much.

# Batch usage
`wordle-solver` is entirely driven by stdin and stdout, so that inputs can be batched instead. This can be useful, for example, when solving [Quordle](https://www.quordle.com/#/).

For example:
```
$ cat wordle.txt
aiery;aacaa
slump;cpapa
smell;cccca
$ ./wordle-solver < wordle.txt
State[gen#1]: 2315 solutions and 12960 words.
⬜️q ⬜️w ⬜️e ⬜️r ⬜️t ⬜️y ⬜️u ⬜️i ⬜️o ⬜️p
  ⬜️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Initial best guess is "trace".
] Considering guess "aiery" with match ⬜️⬜️🟩⬜️⬜️
State[gen#2]: 64 solutions and 352 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬜️u ⬛️i ⬜️o ⬜️p
  ⬛️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Computing entropy...................................
Recommending guess "slump" out of 1 words ("slump") with highest two-level entropy 4.13722 and highest score 15.
] Considering guess "slump" with match 🟩🟨⬜️🟨⬜️
State[gen#3]: 2 solutions and 3 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬛️u ⬛️i ⬜️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
Solutions and associated entropy:
	"smelt": 0.693147
	"smell": 0
Recommending guess "wootz" out of 4 words ("wootz", "whoot", "tondo", "thoft") with highest two-level entropy 0.693147 and highest score 15.
] Considering guess "smell" with match 🟩🟩🟩🟩⬜️
State[gen#4]: 1 solutions and 1 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬛️u ⬛️i ⬜️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
THE solution: "smelt"
] $
```

# Additional interactive commands

There are a few additional interactive commands to help explore the space of solutions.

## Comments

Any line whose first character is a `#` sign is a comment. This may prove useful for batch operation.
```
$ cat wordle.txt
aiery;aacaa
slump;cpapa
# this next guess is not what was recommended by the solver. It's still a "better" guess.
smell;cccca
```

## Reset

During an interactive or batch session, the state of the solver can be reset. This may be useful to run several sessions sequentially, e.g. for [Quordle](https://www.quordle.com/#/) or [Absurdle](https://qntm.org/files/absurdle/absurdle.html).

The syntax for the reset command is a lone `!` at the prompt.

```
$ ./wordle-solver 
State[gen#1]: 2315 solutions and 12960 words.
⬜️q ⬜️w ⬜️e ⬜️r ⬜️t ⬜️y ⬜️u ⬜️i ⬜️o ⬜️p
  ⬜️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Initial best guess is "trace".
] aiery;aacaa
Considering guess "aiery" with match ⬜️⬜️🟩⬜️⬜️
State[gen#2]: 64 solutions and 352 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬜️u ⬛️i ⬜️o ⬜️p
  ⬛️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Computing entropy...................................
Recommending guess "slump" out of 1 words ("slump") with highest two-level entropy 4.13722 and highest score 15.
] !
# RESET!
State[gen#1]: 2315 solutions and 12960 words.
⬜️q ⬜️w ⬜️e ⬜️r ⬜️t ⬜️y ⬜️u ⬜️i ⬜️o ⬜️p
  ⬜️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
] trace;paaap
Considering guess "trace" with match 🟨⬜️⬜️⬜️🟨
State[gen#2]: 58 solutions and 343 words.
⬜️q ⬜️w 🟨e ⬛️r 🟨t ⬜️y ⬜️u ⬜️i ⬜️o ⬜️p
  ⬛️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬛️c ⬜️v ⬜️b ⬜️n ⬜️m 
Computing entropy...................................
Recommending guess "snipy" out of 6 words ("snipy", "ludos", "buhls", "ouphs", "gusli", "pushy") with highest two-level entropy 4.06044 and highest score 15.
] 
```

## Back

The back command (`^`) is used to go "back" one generation.

```
λ ./wordle-solver 
State[gen#1]: 2315 solutions and 12960 words.
⬜️q ⬜️w ⬜️e ⬜️r ⬜️t ⬜️y ⬜️u ⬜️i ⬜️o ⬜️p
  ⬜️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Initial best guess is "trace".
] aiery;aacaa
Considering guess "aiery" with match ⬜️⬜️🟩⬜️⬜️
State[gen#2]: 64 solutions and 352 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬜️u ⬛️i ⬜️o ⬜️p
  ⬛️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Computing entropy...................................
Recommending guess "slump" out of 1 words ("slump") with highest two-level entropy 4.13722 and highest score 15.
] slump;cpapa
Considering guess "slump" with match 🟩🟨⬜️🟨⬜️
State[gen#3]: 2 solutions and 3 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬛️u ⬛️i ⬜️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
Solutions and associated entropy:
	"smelt": 0.693147
	"smell": 0
Recommending guess "thoft" out of 4 words ("wootz", "whoot", "tondo", "thoft") with highest two-level entropy 0.693147 and highest score 15.
] wootz;aaapa
Considering guess "wootz" with match ⬜️⬜️⬜️🟨⬜️
State[gen#4]: 1 solutions and 1 words.
⬜️q ⬛️w 🟨e ⬛️r 🟨t ⬛️y ⬛️u ⬛️i ⬛️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬛️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
THE solution: "smelt"
] ^
^ BACK ONE
State[gen#3]: 2 solutions and 3 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬛️u ⬛️i ⬜️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Solutions and associated entropy:
	"smelt": 0.693147
	"smell": 0
Recommending guess "wootz" out of 4 words ("wootz", "whoot", "tondo", "thoft") with highest two-level entropy 0.693147 and highest score 15.
] whoot;aaaac
Considering guess "whoot" with match ⬜️⬜️⬜️⬜️🟩
State[gen#4]: 1 solutions and 1 words.
⬜️q ⬛️w 🟨e ⬛️r 🟨t ⬛️y ⬛️u ⬛️i ⬛️o ⬛️p
  ⬛️a 🟨s ⬜️d ⬜️f ⬜️g ⬛️h ⬜️j ⬜️k 🟨l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n 🟨m 
Computing entropy...................................
THE solution: "smelt"
] 
```

## Entropy-of

At any given state, the computed entropy of any given word can be queried. This is "lifting the curtain" a bit on the (otherwise not particularly useful) internal details of the solver, but since it's there, it might as well be used.

```State[gen#2]: 64 solutions and 352 words.
⬜️q ⬜️w 🟨e ⬛️r ⬜️t ⬛️y ⬜️u ⬛️i ⬜️o ⬜️p
  ⬛️a ⬜️s ⬜️d ⬜️f ⬜️g ⬜️h ⬜️j ⬜️k ⬜️l
     ⬜️z ⬜️x ⬜️c ⬜️v ⬜️b ⬜️n ⬜️m 
Recommending guess "slump" out of 1 words ("slump") with highest two-level entropy 4.13722 and highest score 15.
] ?smell
[0] H("smell") = 2.31953
[0]H2("smell") = 0
```
The output of the command is somewhat opaque. H is the entropy within the current state, while H2 is the two-level entropy. H2 can be 0 for some words as only the top-entropy words have their two-level entropy computed.

This command is probably not going to stay for long.

# Building

`wordle-solver` is written in C++20. The code is built with a simplistic Makefile and it does not attempt installing.

To build, run `make`:
```
$ make
c++  -std=c++20   -c -o wordle-solver.o wordle-solver.cpp
c++  -std=c++20   -c -o match.o match.cpp
c++  -std=c++20   -c -o state.o state.cpp
c++  -std=c++20   -c -o threadpool.o threadpool.cpp
c++  -std=c++20   -c -o wordlist.o wordlist.cpp
c++   wordle-solver.o match.o state.o threadpool.o wordlist.o   -o wordle-solver
```

To clean, run `make clean`:
```
$ make clean
rm -f match.o state.o threadpool.o wordlist.o wordle-solver.o wordle-solver
```

To run, run `./wordle-solver`.

Good luck!
