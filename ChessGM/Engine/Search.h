/*
*	ChessGM, a free UCI / Xboard chess engine
*	Copyright (C) 2023 Ilyin Yegor
*
*	ChessGM is free software : you can redistribute it and /or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	ChessGM is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with ChessGM. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include "Chess/Board.h"
#include "Limits.h"

/*
*	Search(.h/.cpp) contains the functions responsible for the most important part
*	of the engine - the best move searching algorithm.
* 
*	Currently implemented methods:
*		1) NegaMax - the basic search algorithm
*		2) AlphaBeta Pruning - the basic, failproof pruning algorithm
*		3) Quiescence search for captures/promotions/check evasions/some checks
*		4) SEE pruning in qiescence
*		5) Delta pruning in quiescence
*		6) 2-tier transposition table
*		7) MVV/LVA non-quiets sort
*		8) History heuristic
*		9) Killer moves
*		10) Principal Variation Search
*		11) Futility Pruning
*		12) Razoring
*		13) Null Move Pruning
*		14) Mate Distance Pruning
*		15) Late Move Reduction
*		16) Shallow Depth SEE Pruning
*		17) History Leaf Pruning
*		18) Aspiration Window
*		19) Internal Iterative Deepening
*/

namespace engine {
	enum class NodeType : ufast8 {
		NON_PV = 0,
		PV
	};

	struct SearchResult final {
		Move best;
		Value value;
	};

	struct SearchStack final {
		Move firstKiller;
		Move secondKiller;
	};

	extern Limits g_limits;


	///  SEARCH FUNCTIONS  ///

	// Performance test
	NodesCount perft(Board& board, const Depth depth);

	// The main search function used to find the best move
	SearchResult rootSearch(Board& board);

	// The general search function
	template<NodeType NT = NodeType::PV>
	Value search(Board& board, Value alpha, Value beta, Depth depth, Depth ply);

	// Quiescence search, looks only for captures/some other critical moves
	// It allows to solve the problem of search horizon
	template<NodeType NT = NodeType::PV>
	Value quiescence(Board& board, Value alpha, Value beta, Depth ply, Depth qply);

	///  AUXILIARY FUNCTIONS  ///

	// Initialization before a new game
	void initSearch();

	// When called - stops all searches
	// Expected to be used when a command was given to stop thinking
	void stopSearching();
}