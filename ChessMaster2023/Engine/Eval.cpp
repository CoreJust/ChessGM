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

#include "Eval.h"
#include "PawnHashTable.h"

namespace engine {
// Evaluation by side
	template<Color::Value Side>
	CM_PURE Score evalSide(Board& board) {
		constexpr Color::Value OppositeSide = Color(Side).getOpposite().value();
		constexpr Direction::Value Up = Direction::makeRelativeDirection(Side, Direction::UP).value();
		constexpr Direction::Value Down = Direction::makeRelativeDirection(Side, Direction::DOWN).value();

		Score result = board.scoreByColor(Side);
		const BitBoard occ = board.allPieces();


		///  PAWNS   ///

		const PawnHashEntry& entry = PawnHashTable::getOrScanPHE(board);
		
		// Everything related purely to pawns is pre-evaluated
		result += entry.pawnEvaluation[Side];

		// Passed
		BitBoard pieces = entry.passed.b_and(entry.pawns[Side]);
		BB_FOR_EACH(sq, pieces) {
			// Rook behind a passed
			if (BitBoard rooksBehind = board.byPiece(Piece(Side, PieceType::ROOK)).b_and(BitBoard::directionBits<Down>(sq)); rooksBehind) {
				Square rookSq = Side == Color::WHITE ? rooksBehind.msb() : rooksBehind.lsb();
				if (occ.b_and(BitBoard::betweenBits(sq, rookSq)) == BitBoard::EMPTY) { // Nothing between the rook and the passed
					result += scores::ROOK_BEHIND_PASSED_PAWN;
				}
			}

			// Blocked passed
			if (board[sq.shift(Up)] == Piece(OppositeSide, PieceType::KNIGHT) || board[sq.shift(Up)] == Piece(OppositeSide, PieceType::BISHOP)) {
				result += scores::MINOR_PASSED_BLOCKED;
			}
		}

		return result;
	}

	Value eval(Board& board) {
		Score score = evalSide<Color::WHITE>(board) - evalSide<Color::BLACK>(board);

		///  RESULTS  ///

		Material material = board.materialByColor(Color::WHITE) + board.materialByColor(Color::BLACK);
		Value result = score.collapse(material);
		result *= (-1 + 2 * (board.side() == Color::WHITE));

		return result + scores::TEMPO_SCORE.collapse(material);
	}
}