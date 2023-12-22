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

#include "Search.h"
#include <utility>
#include <atomic>
#include <algorithm>

#include "Utils/IO.h"
#include "Eval.h"
#include "Engine.h"
#include "MovePicker.h"
#include "TranspositionTable.h"

namespace engine {
	// Constants

	constexpr Value DELTA_PRUNING_MARGIN = 200;

	constexpr Depth MAX_QPLY_FOR_CHECKS = 2;
	constexpr Depth MIN_NULLMOVE_DEPTH = 2;
	constexpr Depth NULLMOVE_DEPTH_REDUCTION_BASE = 3;
	constexpr Depth MIN_NULLMOVE_VERIFICATION_DEPTH = 5;
	constexpr Depth MIN_LMR_DEPTH = 3;
	constexpr Depth MAX_LOW_DEPTH_SEE_PRUNING_DEPTH = 3;

	constexpr Depth NULLMOVE_HIGH_DEPTH_DENOMINATOR = 5;
	constexpr Value NULLMOVE_BETA_DIFFERENCE_DENOMINATOR = 300;
	constexpr Value LMR_MAX_HISTORY_SUCCESS_RATE = 75;
	constexpr u8 LMR_MIN_QUIETS_COUNT = 2;
	constexpr Depth LMR_HIGH_DEPTH_DENOMINATOR = 9;
	constexpr u8 LMR_MANY_QUIETS_DENOMINATOR = 9;


	// Global variables
	std::atomic_bool g_mustStop = false; // Must the search stop?

	NodesCount g_nodesCount = 0; // Nodes during the current search
	SearchStack g_searchStacks[2 * MAX_DEPTH + 2];
	MoveList g_moveLists[2 * MAX_DEPTH];
	MoveList _g_PVs[2 * MAX_DEPTH + 1];
	MoveList* g_PVs = _g_PVs + 1; // So as to access the ply = -1 (for root moves)

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

		Move lastBest;
		Value lastResult = -INF;
		Depth rootDepth = 0;

		// Initializing the search
		g_mustStop = false;
		g_nodesCount = 0;

		MovePicker::resetHistoryTables();
		TranspositionTable::setRootAge(board.moveCount());

		memset(g_searchStacks, 0, sizeof(g_searchStacks));

		// Looking for the best move
		board.generateMoves(moves);
		while (!g_limits.isDepthLimitBroken(++rootDepth)) {
			Move best;
			Value result = -INF;
			u8 legalMoves = 0;

			g_PVs[-1].clear();
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
					g_PVs[-1].mergeWith(g_PVs[0], 0);
				}

				if (g_mustStop) {
					if (lastBest.isNullMove()) { // In case we haven't finished the first iteration
						return SearchResult { .best = best, .value = result };
					} else {
						return SearchResult { .best = lastBest, .value = lastResult };
					}
				}
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

					io::g_out << " pv " << g_PVs[-1].toString(best) << std::endl;
				} else { // Xboard/Console
					io::g_out << rootDepth << ' '
						<< result << ' '
						<< g_limits.elapsedCentiseconds() << ' '
						<< g_nodesCount << ' '
						<< g_PVs[-1].toString(best) << std::endl;
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


		///  MATE DISTANCE PRUNING  ///

		if constexpr (NT != NodeType::PV) {
			alpha = std::max(alpha, Value(-MATE + ply));
			beta = std::min(beta, Value(MATE - ply));

			if (alpha >= beta) {
				return alpha;
			}
		}


		///  TRANSPOSITION TABLE  ///

		TableEntry* entry = TranspositionTable::probe(board.computeHash());
		Move tableMove = Move::makeNullMove();
		if (entry != nullptr) { // Current position was found
			// Check if it is possible to just return the value from the table
			if (entry->depth >= depth && ply && (entry->isPvNode() || NT != NodeType::PV)) {
				Value value = entry->value;
				if (isMateValue(value)) { // Fix the mate distance
					if (value > MATE - 2 * MAX_DEPTH) {
						value -= ply;
					} else if (value < -MATE + 2 * MAX_DEPTH) {
						value += ply;
					}
				}

				switch (entry->getBoundType()) {
					case EntryType::EXACT: return value;
					case EntryType::ALPHA: 
						if (value <= alpha) {
							return alpha;
						} break;
					case EntryType::BETA:
						if (value >= beta) {
							return beta;
						} break;
				default: break;
				}
			}

			tableMove = Move::fromData(entry->move);
		}


		///  PRUNINGS AND REDUCTIONS  ///

		const bool isInCheck = board.isInCheck();
		if (NT != NodeType::PV && !isInCheck) {
			const static Value FUTILITY_MARGIN[] = { 0, 50, 200, 400, 700 };

			Value staticEval = eval(board);


			///  FUTILITY PRUNING  ///

			if (depth <= 4) {
				const Value margin = FUTILITY_MARGIN[depth];

				if (staticEval <= alpha - margin) {
					return quiescence(board, alpha, beta, ply, 0);
				} if (staticEval >= beta + margin) {
					return beta;
				}
			}


			/// NULL MOVE  ///

			if (staticEval >= beta 
				&& depth >= MIN_NULLMOVE_DEPTH 
				&& board.hasNonPawns(board.side())) {
				Depth R = NULLMOVE_DEPTH_REDUCTION_BASE 
					+ (depth - MIN_NULLMOVE_DEPTH) / NULLMOVE_HIGH_DEPTH_DENOMINATOR 
					+ std::max((staticEval - beta) / NULLMOVE_BETA_DIFFERENCE_DENOMINATOR, 0);

				if (R < 0) {
					R = 0;
				}

				board.makeNullMove();
				Value tmp = -search<NodeType::NON_PV>(board, -beta, -beta + 1, depth - R, ply + 1);
				board.unmakeNullMove();

				if (g_mustStop) {
					return alpha;
				}

				if (tmp >= beta) {
					if (isMateValue(tmp)) {
						tmp = beta;
					}

					if (depth >= MIN_NULLMOVE_VERIFICATION_DEPTH) { // Verifying the results
						Value verification = search<NodeType::NON_PV>(board, beta - 1, beta, depth - R, ply);

						if (verification >= beta) {
							return tmp;
						}
					} else {
						return tmp;
					}
				}
			}
		}


		///  RECURSIVE SEARCH  ///

		u8 legalMovesCount = 0;
		u8 quietMovesCount = 0;
		EntryType entryType = EntryType::ALPHA;
		Move bestMove = Move::makeNullMove();

		SearchStack* ss = &g_searchStacks[ply];
		ss[2].firstKiller = ss[2].secondKiller = Move::makeNullMove();

		MoveList& moves = g_moveLists[ply];
		board.generateMoves(moves);

		MovePicker picker(board, moves, ply, tableMove, ss);
		while (picker.hasMore()) {
			const Move m = picker.pick();
			if (!board.isLegal(m)) {
				continue;
			}

			++legalMovesCount;

			const bool isQuiet = board.isQuiet(m);
			if (NT != NodeType::PV && depth <= MAX_LOW_DEPTH_SEE_PRUNING_DEPTH && !isInCheck && board.hasNonPawns(board.side())) {


				///  LOW DEPTH SEE PRUNING  ///

				if (board.SEE(m) <= -scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE] * depth) {
					continue; // Skip losing moves at low depth
				}


				///  HISTORY LEAF PRUNING  ///

				if (isQuiet && ++quietMovesCount > LMR_MIN_QUIETS_COUNT) {
					const static Value MAX_SUCCESS_RATE[] = { 0, 20, 12, 7, 3 };

					const Value historySuccessRate = MovePicker::getHistoryValue(board[m.getFrom()], m.getTo());
					if (historySuccessRate < MAX_SUCCESS_RATE[depth] && !board.givesCheck(m)) {
						continue;
					}
				}
			}

			if (isQuiet && !isInCheck) { // Updating the history
				MovePicker::addHistoryTry(board, m, depth);
			}

			++g_nodesCount;
			board.makeMove(m);


			///  LATE MOVE REDUCTIONS  ///

			Depth reduction = 0;
			if (depth >= MIN_LMR_DEPTH
				&& !isInCheck
				&& !board.isInCheck() // does not gives check
				&& isQuiet) {
				const Value historySuccessRate = MovePicker::getHistoryValue(board[m.getTo()], m.getTo());

				if (historySuccessRate < LMR_MAX_HISTORY_SUCCESS_RATE && ++quietMovesCount > LMR_MIN_QUIETS_COUNT) {
					reduction = 1 
						+ (depth - MIN_LMR_DEPTH) / LMR_HIGH_DEPTH_DENOMINATOR 
						+ (quietMovesCount - LMR_MIN_QUIETS_COUNT) / LMR_MANY_QUIETS_DENOMINATOR;

					if (historySuccessRate > 50) {
						--reduction;
					} else if (historySuccessRate < 10) {
						++reduction;

						if (historySuccessRate < 2) {
							++reduction;
						}
					}

					if (reduction >= depth) {
						reduction = depth - 1;
					}
				}
			}


			///  PRINCIPAL VARIATION SEARCH  ///

			Value tmp;
			if (legalMovesCount == 1) {
				tmp = -search<NT>(board, -beta, -alpha, depth - 1, ply + 1);
			} else {
				tmp = -search<NodeType::NON_PV>(board, -alpha - 1, -alpha, depth - reduction - 1, ply + 1);
				if (tmp > alpha && reduction) { // LMR failed
					tmp = -search<NodeType::NON_PV>(board, -alpha - 1, -alpha, depth - 1, ply + 1);
				} if (NT == NodeType::PV && tmp > alpha && tmp < beta) { // Full window search
					tmp = -search<NodeType::PV>(board, -beta, -alpha, depth - 1, ply + 1);
				}
			}

			board.unmakeMove(m);
			if (g_mustStop) {
				return alpha;
			}


			///  ALPHA-BETA PRUNING  ///

			if (tmp > alpha) {
				alpha = tmp;
				entryType = EntryType::EXACT;
				bestMove = m;

				// Updating the PV
				if constexpr (NT == NodeType::PV) {
					g_PVs[ply].clear();
					g_PVs[ply].push(m);
					g_PVs[ply].mergeWith(g_PVs[ply + 1], 1);
				}
			}

			if (alpha >= beta) { // The actual pruning
				if (isQuiet && !isInCheck) { // Updating the history
					MovePicker::addHistorySuccess(board, m, depth);
					if (ss->firstKiller.getData() != m.getData()) { // Uodating killers
						ss->secondKiller = std::exchange(ss->firstKiller, m);
					}
				}

				entryType = EntryType::BETA;
				break;
			}
		}

		if (legalMovesCount == 0) {
			alpha = board.isInCheck() 
				? -MATE + ply // Mate
				: 0; // Stalemate
		}

		// Saving the results in the transposition table
		TranspositionTable::tryRecord(
			EntryType(u8(entryType) | u8(NT)), 
			board.computeHash(), 
			bestMove.getData(), 
			alpha, 
			board.moveCount(), 
			depth,
			ply
		);

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


			///  STANDING PAT  ///

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

		MovePicker picker(board, moves, ply);

		// Iterative search
		while (picker.hasMore()) {
			const Move m = picker.pick();
			if (!board.isLegal(m)) {
				continue;
			}

			++legalMovesCount;

			if (!isInCheck && board.hasNonPawns(board.side())) { // So as not to prune in endgame


				///  DELTA PRUNING  ///

				// Idea: if with the value of the captured piece, even with a surplus margin
				//		 cannot improve the value, than the move is unlikely to improve the alpha as well.
				// Promotions are not considered here
				if (m.getMoveType() != MoveType::PROMOTION) {
					Value capturedValue = scores::SIMPLIFIED_PIECE_VALUES[
						m.getMoveType() == MoveType::ENPASSANT ? Piece::PAWN_WHITE : board[m.getTo()]
					];

					if (staticEval + capturedValue + DELTA_PRUNING_MARGIN <= alpha && !board.givesCheck(m)) {
						continue;
					}
				}


				///  SEE PRUNING  ///
				
				// Checks if the move can lead to any benefit
				// If not, than we can likely safely skip it
				if (board.SEE(m) < 0) {
					continue;
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

	void initSearch() {
		MovePicker::init();
	}

	void stopSearching() {
		g_mustStop = true;
	}
}