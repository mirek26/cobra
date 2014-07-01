
Cobra is a tool for code-breaking game (deductive game) analysis, 
which has been developed as a part of my master thesis 
that be found in thesis/main.pdf.
Source codes are located in cobra/ directory.

## Deductive games

Deductive games are two-player games in which the first player selects a code from a given set and the second player strives to reveal it using a minimal number of experiments. A prominent example of a code-breaking game is the board game Mastermind, where the codebreaker tries to guess a combination of coloured pegs. There are many natural questions to ask about code-breaking games. What strategy for experiment selection should the codebreaker use? Can we compute a lower and upper bound for the number of experiments needed to reveal the code? Is it possible to compute an optimal strategy? What is the performance of a given heuristic? Much research on the topic has been done but the authors usually focus on one particular game and little has been written about code-breaking games in general. We create a general model of code-breaking games based on propositional logic and design a computer language for game specification. Further, we suggest general algorithms for game analysis and strategy synthesis and implement them in a computer program named Cobra. Using Cobra, we can reproduce existing results for Mastermind, analyse new code-breaking games and easily evaluate new heuristics for experiment selection.




