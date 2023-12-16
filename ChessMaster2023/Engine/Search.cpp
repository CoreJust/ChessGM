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
#include <algorithm>

#include "Utils/IO.h"
#include "Search.h"
#include "Eval.h"
#include "Engine.h"

namespace engine {
	// Constants

	constexpr Value DELTA_PRUNING_MARGIN = 200;

	constexpr Depth MAX_QPLY_FOR_CHECKS = 2;


	// Global variables
	std::atomic_bool g_mustStop = false; // Must the search stop?

	NodesCount g_nodesCount = 0; // Nodes during the current search
	MoveList g_moveLists[2 * MAX_DEPTH];
	MoveList g_PVs[2 * MAX_DEPTH];

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
		static MoveList pv;

		Move lastBest;
		Value lastResult = -INF;
		Depth rootDepth = 0;

		// Initializing the search
		g_mustStop = false;
		g_nodesCount = 0;

		// Looking for the best move
		board.generateMoves(moves);
		while (!g_limits.isDepthLimitBroken(++rootDepth)) {
			Move best;
			Value result = -INF;
			u8 legalMoves = 0;

			pv.clear();
			for (Move &m : moves) {
				if (!board.isLegal(m)) {
					continue;
				}

				++legalMoves;
				board.makeMove(m);
				Value tmp = -search<NodeType::PV>(board, -INF, -result, rootDepth - 1, 0);
				board.unmakeMove(m);

				m.setValue(tmp);
				if (tmp > result) {
					result = tmp;
					best = m;

					// Copying the PV
					pv.mergeWith(g_PVs[0], 0);
				}

				if (g_mustStop) {
					if (lastBest.isNullMove()) { // In case we haven't finished the first iteration
						return SearchResult { .best = best, .value = result };
					} else {
						return SearchResult { .best = lastBest, .value = lastResult };
					}
				}
			}

			if (legalMoves == 0) { // Such situation is highly unlikely, but it is possible that we have no legal moves somehow
				return SearchResult { .best = Move::makeNullMove(), .value = Value(board.isInCheck() ? -MATE : Value(0)) };
			} else if (legalMoves == 1) { // We have a single reply, so no need to search any deeper
				return SearchResult { .best = best, .value = result };
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

					io::g_out << " pv " << pv.toString(best) << std::endl;
				} else { // Xboard/Console
					io::g_out << rootDepth << ' '
						<< result << ' '
						<< g_limits.elapsedCentiseconds() << ' '
						<< g_nodesCount << ' '
						<< pv.toString(best) << std::endl;
				}
			}

			lastBest = best;
			lastResult = result;

			// Sorting the moves
			std::sort(moves.begin(), moves.end(), [](Move a, Move b) -> bool { return a.getValue() < b.getValue(); });
		}

		return SearchResult { .best = lastBest, .value = lastResult };
	}

	// The general search function
	template<NodeType NT>
	Value search(Board& board, Value alpha, Value beta, Depth depth, Depth ply) {
		// Reached the leaf node (all the checks would be done within qsearch)
		if (depth <= 0) {
			return quiescence<NT>(board, alpha, beta, ply, 0);
		}

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

		if constexpr (NT == NodeType::PV) {
			g_PVs[ply].clear();
		}

		// Check if the game ended in a draw
		if (board.isDraw(ply)) {
			return 0;
		}

		// Check if we have reached the maximal possible ply
		if (ply > MAX_DEPTH) {
			return alpha;
		}

		// The recursive search
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
			Value tmp = -search<NT>(board, -beta, -alpha, depth - 1, ply + 1);
			board.unmakeMove(m);

			if (g_mustStop) {
				return alpha;
			}

			// Alpha-Beta Pruning
			if (tmp > alpha) {
				alpha = tmp;

				// Updating the PV
				if constexpr (NT == NodeType::PV) {
					g_PVs[ply].clear();
					g_PVs[ply].push(m);
					g_PVs[ply].mergeWith(g_PVs[ply + 1], 1);
				}
			}

			if (alpha >= beta) { // The actual pruning
				break;
			}
		}

		if (legalMovesCount == 0) {
			return board.isInCheck() 
				? -MATE + ply // Mate
				: 0; // Stalemate
		}

		return alpha;
	}

	template<NodeType NT>
	Value quiescence(Board& board, Value alpha, Value beta, Depth ply, Depth qply) {
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

		if constexpr (NT == NodeType::PV) {
			g_PVs[ply].clear();
		}

		// Check if the game ended in a draw
		if (board.isDraw(ply)) {
			return 0;
		}

		// Check if we have reached the maximal possible ply
		if (ply > MAX_DEPTH) {
			return alpha;
		}

		Value staticEval = eval(board);
		if (!board.isInCheck()) {
			// Standing pat
			if (staticEval >= beta) {
				return staticEval;
			}

			if (staticEval > alpha) {
				alpha = staticEval;
			}
		}

		const bool isInCheck = board.isInCheck();
		u8 legalMovesCount = 0;

		// Move generation
		MoveList& moves = g_moveLists[ply];
		board.generateMoves<movegen::CAPTURES>(moves);
		if (!isInCheck && qply < MAX_QPLY_FOR_CHECKS) {
			board.generateMoves<movegen::QUIET_CHECKS>(moves);
		}

		// Iterative search
		for (Move m : moves) {
			if (!board.isLegal(m)) {
				continue;
			}

			++legalMovesCount;

			if (!isInCheck) { 
				if (board.byPieceType(PieceType::PAWN) != BitBoard::EMPTY) { // So as not to prune in endgame
					///  Delta pruning  ///

					// Idea: if with the value of the captured piece, even with a surplus margin
					//		 cannot improve the value, than the move is unlikely to improve the alpha as well.
					// Promotions are not considered here
					if (m.getMoveType() != MoveType::PROMOTION) {
						Value capturedValue = scores::SIMPLIFIED_PIECE_VALUES[
							m.getMoveType() == MoveType::ENPASSANT ? Piece::PAWN_WHITE : board[m.getTo()]
						];

						// TODO: swap conditions
						if (!board.givesCheck(m) && staticEval + capturedValue + DELTA_PRUNING_MARGIN <= alpha) {
							continue;
						}
					}

					///  SEE pruning  ///

					// Checks if the move can lead to any benefit
					// If not, than we can likely safely skip it
					if (board.SEE(m) < 0) {
						continue;
					}
				}
			}

			++g_nodesCount;
			board.makeMove(m);
			Value tmp = -quiescence<NT>(board, -beta, -alpha, ply + 1, qply + 1);
			board.unmakeMove(m);

			if (g_mustStop) {
				return alpha;
			}

			// Alpha-Beta Pruning
			if (tmp > alpha) {
				alpha = tmp;

				// Updating the PV
				if constexpr (NT == NodeType::PV) {
					g_PVs[ply].clear();
					g_PVs[ply].push(m);
					g_PVs[ply].mergeWith(g_PVs[ply + 1], 1);
				}
			}

			if (alpha >= beta) { // The actual pruning
				break;
			}
		}

		if (legalMovesCount == 0 && board.isInCheck()) {
			return -MATE + ply;
		}

		return alpha;
	}

	void stopSearching() {
		g_mustStop = true;
	}
}