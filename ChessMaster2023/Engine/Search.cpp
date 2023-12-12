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

#include <atomic>

#include "Utils/IO.h"
#include "Search.h"
#include "Eval.h"
#include "Engine.h"

namespace engine {
	// Global variables
	std::atomic_bool g_mustStop = false; // Must the search stop?

	NodesCount g_nodesCount = 0; // Nodes during the current search
	MoveList g_moveLists[2 * MAX_DEPTH];

	Limits g_limits;


	///  SEARCH FUNCTIONS  ///

	NodesCount perft(Board& board, const Depth depth) {
		NodesCount result = 0;
		MoveList& moves = g_moveLists[depth];

		board.generateMoves(moves);
		for (Move m : moves) {
			if (!board.isLegal(m)) {
				continue;
			}

			board.makeMove(m);
			if (depth <= 1) {
				result++;
			} else {
				result += perft(board, depth - 1);
			}

			board.unmakeMove(m);
		}

		return result;
	}

	SearchResult rootSearch(Board& board) {
		static MoveList moves;
		Move best, lastBest;
		Value result = -INF, lastResult = -INF;
		Depth rootDepth = 0;

		// Initializing the search
		g_mustStop = false;
		g_nodesCount = 0;

		// Looking for the best move
		board.generateMoves(moves);
		while (!g_limits.isDepthLimitBroken(++rootDepth)) {
			u8 legalMoves = 0;
			for (Move m : moves) {
				if (!board.isLegal(m)) {
					continue;
				}

				++legalMoves;
				board.makeMove(m);
				Value tmp = -search<NodeType::PV>(board, -INF, INF, rootDepth - 1, 0);
				board.unmakeMove(m);

				if (g_mustStop) {
					return SearchResult { .best = lastBest, .value = lastResult };
				}

				if (tmp > result) {
					result = tmp;
					best = m;
				}
			}

			// Such situation is highly unlikely, but it is possible that we have no legal moves somehow
			if (legalMoves == 0) {
				return SearchResult { .best = Move::makeNullMove(), .value = Value(board.isInCheck() ? -MATE : Value(0)) };
			}

			// Check if we reached the soft limit
			// Here is the perfect place to stop search
			if (g_limits.isSoftLimitBroken()) {
				return SearchResult { .best = best, .value = result };
			}

			// Printing the current search state
			if (options::g_postMode) {
				if (io::getMode() == io::IOMode::UCI) {
					io::g_out 
						<< "info depth " << rootDepth
						<< " nodes " << g_nodesCount
						<< " time " << g_limits.elapsedMilliseconds();

					if (isMateValue(result)) {
						io::g_out << " score mate " << (result < 0 ? -gettingMatedIn(result) : givingMateIn(result));
					} else {
						io::g_out << " score cp " << result;
					}

					io::g_out << " pv " << best << std::endl;
				} else { // Xboard/Console
					io::g_out << rootDepth << ' '
						<< result << ' '
						<< g_limits.elapsedCentiseconds() << ' '
						<< g_nodesCount << ' '
						<< best << std::endl;
				}
			}

			lastBest = best;
			lastResult = result;
			result = -INF;
		}

		return SearchResult { .best = lastBest, .value = lastResult };
	}

	// The general search function
	template<NodeType NT>
	Value search(Board& board, Value alpha, Value beta, Depth depth, Depth ply) {
		if (g_mustStop) {
			return alpha;
		}

		// Checking limits and input
		if ((g_nodesCount & 0x1ff) == 0) {
			if (g_limits.isHardLimitBroken() || g_limits.isNodesLimitBroken(g_nodesCount)) {
				g_mustStop = true;
				return alpha;
			}

			// Cheching for possible input once in 8192 nodes
			if ((g_nodesCount & 0x1fff) == 0) {
				checkInput();
			}
		}

		// Check if we have reached the maximal possible ply
		if (ply > MAX_DEPTH) {
			return alpha;
		}

		// Check if the game ended in a draw
		if (board.isDraw(ply)) {
			return 0;
		}

		// Reached the leaf node
		if (depth <= 0) {
			return eval(board);
		}

		// The recursive search
		Value result = alpha;
		u8 legalMovesCount = 0;
		MoveList& moves = g_moveLists[ply];

		board.generateMoves(moves);
		for (Move m : moves) {
			if (!board.isLegal(m)) {
				continue;
			}

			++legalMovesCount;
			++g_nodesCount;

			board.makeMove(m);
			Value tmp = -search<NT>(board, -beta, -result, depth - 1, ply + 1);
			board.unmakeMove(m);

			if (g_mustStop) {
				return alpha;
			}

			// Alpha-Beta Pruning
			if (tmp > result) {
				result = tmp;
			}

			if (result >= beta) { // The actual pruning
				break;
			}
		}

		if (legalMovesCount == 0) {
			return board.isInCheck() 
				? -MATE + ply // Mate
				: 0; // Stalemate
		}

		return result;
	}

	template Value search<NodeType::PV>(Board& board, Value alpha, Value beta, Depth depth, Depth ply);

	void stopSearching() {
		g_mustStop = true;
	}
}