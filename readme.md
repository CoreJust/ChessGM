# Overview
ChessGM is a chess engine written in C++. It supports both Winboard and UCI and has its own console interface. 

The engine is based on my old project (that I have never published), some of the auxiliary functions (mostly move generation, the magic bitboards) are based on [Stockfish](https://github.com/official-stockfish/Stockfish/tree/master).

The engine is developed as a hobby.

## Features
* Search:
1) NegaMax with Alpha-Beta
2) MVV/LVA move ordering
3) History heuristic
4) Killer moves
5) 2-tier transposition table
6) Principal Variation Search
7) Futility Pruning
8) Razoring
9) Null Move Pruning with verification
10) Late Move Reduction
11) Shallow Depth SEE Pruning
12) History Leaf Pruning
13) Mate Distance Pruning
14) Aspiration Window
15) Internal Iterative Deepening

* Quiescence search:
1) Captures, promotions, checks and check evasions
2) SEE pruning
3) Delta pruning

* Static evaluation:
1) Material (Separate pieces, Bishop Pair)
2) Piece-Square Tables
3) Tempo
4) Pawns (backward, isolated, double, defended pawns, pawn islands)
5) Passed pawns (their rank, tarrasch rule, minor blocking a passed)
6) Pawn hash table
7) Separate evaluation functions for some endgames (KXK, KPsKPs, KBNK, drawish endgames)

## Engine power
ChessGM was tested in a tournament against several other engines from CCRL
with time control 0:01+1.

Estimated elo: 2434 (computed with Bayeselo).

## Contents
* ChessMaster2023/ - the source files
* ChessMaster2023.sln - Visual Studio Solution
* ChessMaster2023.exe - executable for Windows
* Makefile - can be used to build the engine with GCC
* changelog.txt - the history of versions
* LICENSE - the license (GNU GPL)


# Building
There is a windows binary provided. The project is made in Visual Studio and fully supports MSVC, for MSVC there is a VS solution file. Also, GNU GCC is supported.
To build with GCC, a Makefile is provided.

# Roadmap
The features that are supposed to be implemented by the future versions (most of which were implemented in the old ChessGM of mine):

* 0.7) Pieces update (mobility, space, connectivity, center control, trapped pieces...)
* 0.7.1) Knights and bishops (outposts, bad bishop, fianchetto, color weakness
* 0.7.2) Rooks and queens (rook on (semi)open file, rook behind a passed, rook on seventh rank,
	   paired rooks, queens penalty for early development, tropism?)
* 0.8) Extensions (single move extension, check extension, capture/recapture extension,
	   passed pawn extension, PV extension, singular extension...)
* 0.9) King update (mate at a glance, pins/x-rays, castlings (rights), pawn shield, pawn storm, tropism,
	 virtual mobility, scaling with material, king zone attack, square control in king zone, king pawn tropism)
* 0.10) General endgame evaluation and search improvement, parallel search
* 1.0) Final improvements before the first release: some small additions, optimization, bugs fixing...)
* 1.1) Evaluation weights search via learning
* 1.2) ???


# License
ChessGM is distributed under [GNU General Public License](https://www.gnu.org/licenses/).
