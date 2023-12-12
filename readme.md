# Overview
ChessMaster2023 is a chess engine written in C++. It supports both Winboard and UCI and has its own console interface. 

The engine is based on my old project (that I have never published), some of the auxiliary functions (mostly move generation, the magic bitboards) are based on [Stockfish](https://github.com/official-stockfish/Stockfish/tree/master).

The engine is developed as a hobby.

## Features
* Search:
1) NegaMax with Alpha-Beta

* Static evaluation:
1) Material
2) Piece-Square Tables

## Engine power
(This is an early version of engine, so its power is low)

ChessMaster was tested in a tournament against several other engines from CCRL
with time control 1+1.

Estimated elo: 900 (computed with Bayeselo).

_The engine is expected to reach at least 2100 elo by the first release - such was the
rating of the original engine of mine._

# Technical infomation
The project is made in Visual Studio and fully supports MSVC, should be fine with Intel. The gcc and clang compilers were not tested, but their support is planned.

# Roadmap
The features that are supposed to be implemented by the first release (most of which were implemented in the old ChessMaster of mine):

* Search:
1) Move picking (with Static Exchange Evaluation)
2) Killer move heuristic
3) History heuristic
4) Hash table
5) Internal Iterative Deepening
6) Quiescence search
7) Extensions (capture, passed pawn, recapture, check, singular...)
8) Null move heuristic
9) Futility pruning
10) Mate Distance pruning
11) Late Move Reduction
12) And some others

* Static evaluation:
1) Tempo
2) Pawn structure
3) Passed pawns (and candidates)
4) Outposts
5) Pieces mobility
6) Material combinations (e.g. two bishops)
7) Pins
8) King safety
9) Endgame-specific evaluations
10) Lazy evaluation
11) And some others


# License
ChessMaster is distributed under [GNU General Public License](https://www.gnu.org/licenses/).
