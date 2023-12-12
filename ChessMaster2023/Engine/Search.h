/*
*	ChessMaster, a free UCI / Xboard chess engine
*	Copyright (C) 2023 Ilyin Yegor
*
*	ChessMaster is free software : you can redistribute it and /or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	ChessMaster is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with ChessMaster. If not, see <https://www.gnu.org/licenses/>.
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
*/

namespace engine {
	enum class NodeType : ufast8 {
		PV = 0,
		NON_PV
	};

	struct SearchResult {
		Move best;
		Value value;
	};

	// Some constants
	constexpr Depth MAX_DEPTH = 99;

	constexpr Value INF = 31000;
	constexpr Value MATE = 30000;

	extern Limits g_limits;


	///  AUXILIARY FUNCTIONS  ///

	CM_PURE constexpr bool isMateValue(const Value value) noexcept {
		return value > MATE - MAX_DEPTH * 2 || value < MAX_DEPTH * 2 - MATE;
	}

	// Moves before the mate
	CM_PURE constexpr Depth givingMateIn(const Value value) noexcept {
		return (MATE + 2 - value) / 2;
	}

	// Moves before the mate
	CM_PURE constexpr Depth gettingMatedIn(const Value value) noexcept {
		return (value + MATE + 1) / 2;
	}


	///  SEARCH FUNCTIONS  ///

	// Performance test
	NodesCount perft(Board& board, const Depth depth);

	// The main search function used to find the best move
	SearchResult rootSearch(Board& board);

	// The general search function
	template<NodeType NT = NodeType::PV>
	Value search(Board& board, Value alpha, Value beta, Depth depth, Depth ply);


	///  AUXILIARY FUNCTIONS  ///

	// When called - stops all searches
	// Expected to be used when a command was given to stop thinking
	void stopSearching();
}