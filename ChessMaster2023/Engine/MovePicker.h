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
#include <cstring>
#include "Chess/Board.h"
#include "Search.h"

/*
*	MovePicker(.h/.cpp) contains the MovePicker class that is used 
*	to score the moves and to get them in order of expected best to 
*	expected worst.
*/

namespace engine {
	// Used in history heuristic
	// s_historyTries is the number of times the move was made during the search
	extern uint32_t s_historyTries[Piece::VALUES_COUNT][Square::VALUES_COUNT];

	// s_historySuccesses is the number of times the move triggered a successful cut
	extern uint32_t s_historySuccesses[Piece::VALUES_COUNT][Square::VALUES_COUNT];

	// Does not generate the moves, only sorts them and picks the best ones.
	class MovePicker final {
	private:
		// Score constants
		constexpr inline static Value SECOND_KILLER = 110;
		constexpr inline static Value FIRST_KILLER = 120;
		constexpr inline static Value CAPTURE = 1000;
		constexpr inline static Value TRANSPOSITION_TABLE = 30000;

		// Other constants
		constexpr inline static u8 HISTORY_RENEWAL_SHIFT = 3;
		constexpr inline static u32 HISTORY_SUCCESS_ADD = 1;
		constexpr inline static u32 HISTORY_TRY_ADD = 2;

	private:
		static SearchStack s_noSS;

	private:
		Move* m_first;
		Move* m_end;

	public:
		// Initializes the move picker and scores the moves
		INLINE MovePicker(
			Board& board,
			MoveList& moves, 
			const Depth ply,
			const Move tableMove = Move::makeNullMove(),
			SearchStack* ss = &s_noSS
		) noexcept : m_first(moves.begin()), m_end(moves.end()) {
			for (Move* move = m_first; move < m_end; ++move) {
				const u16 data = move->getData();

				// Transposition table's move
				if (data == tableMove.getData()) {
					move->setValue(TRANSPOSITION_TABLE);
					continue;
				}

				// Other moves
				if (board.isQuiet(*move)) {
					if (data == ss->firstKiller.getData()) {
						move->setValue(FIRST_KILLER);
					} else if (data == ss->secondKiller.getData()) {
						move->setValue(SECOND_KILLER);
					} else {
						move->setValue(getHistoryValue(board[move->getFrom()], move->getTo()));
					}
				} else { // Capture/promotion
					const Piece piece = board[move->getFrom()];
					const Piece captured = move->getMoveType() == MoveType::ENPASSANT 
						? Piece::PAWN_WHITE 
						: board[move->getTo()];

					const Piece promoted = move->getMoveType() == MoveType::PROMOTION 
						? Piece(Color::WHITE, move->getPromotedPiece()) 
						: Piece::NONE;

					const Value pieceValue = scores::SIMPLIFIED_PIECE_VALUES[piece];
					const Value capturedValue = scores::SIMPLIFIED_PIECE_VALUES[captured];
					const Value promotedValue = scores::SIMPLIFIED_PIECE_VALUES[promoted];

					const Value balance = (capturedValue + promotedValue) * 2 - pieceValue;
					move->setValue(CAPTURE + balance);
				}
			}
		}

		CM_PURE constexpr Move pick() noexcept {
			Move* best = m_first;
			Value bestValue = best->getValue();
			for (Move* move = m_first + 1; move < m_end; ++move) {
				if (move->getValue() > bestValue) {
					best = move;
					bestValue = move->getValue();
				}
			}

			if (best != m_first) {
				std::swap(*m_first, *best);
			}

			return *(m_first++);
		}

		// Returns true if there are still any moves to be picked
		CM_PURE constexpr bool hasMore() const noexcept {
			return m_first < m_end;
		}

	public:
		static void init() noexcept {
			memset(s_historyTries, 0, sizeof(s_historyTries));
			memset(s_historySuccesses, 0, sizeof(s_historySuccesses));
		}

		// Radically decreases the history tables' values
		// Does not clear it completely because history from the last several
		// moves can be partially reused
		static void resetHistoryTables() noexcept {
			for (Piece piece : Piece::iter()) {
				if (piece.getType() == PieceType::NONE) {
					continue;
				}

				for (Square to : Square::iter()) {
					s_historyTries[piece][to] >>= HISTORY_RENEWAL_SHIFT;
					s_historySuccesses[piece][to] >>= HISTORY_RENEWAL_SHIFT;
				}
			}
		}

		INLINE static void addHistoryTry(Board& board, const Move m, const Depth depth) noexcept {
			s_historyTries[board[m.getFrom()]][m.getTo()] += depth * depth;
		}

		INLINE static void addHistorySuccess(Board& board, const Move m, const Depth depth) noexcept {
			s_historySuccesses[board[m.getFrom()]][m.getTo()] += depth * depth;
		}

		// It is used for most quiets
		// The idea is that if move is commonly successful, than it must be good in current position too
		// ADDs are used to differentiate between, for example, 1 try - 1 success and 10 tries - 10 successes
		// It is obvious the second is better, and with the ADDs we would be able to take it into account
		// Also, like this, move that were never tried would initially have a score of 50,
		// which is only natural - an unknown move is likely to be better than those with high failure rate
		CM_PURE static Value getHistoryValue(const Piece piece, const Square to) noexcept {
			return uint64_t(s_historySuccesses[piece][to] + HISTORY_SUCCESS_ADD) * 100 / (s_historyTries[piece][to] + HISTORY_TRY_ADD);
		}
	};
}