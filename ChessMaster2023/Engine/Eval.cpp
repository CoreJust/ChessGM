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
	// Checks if the current position is drawish from the stonger side's POV
	template<Color::Value StrongSide>
	CM_PURE bool isDrawishEndgame(Board& board, const u8 strongMat, const u8 weakMat) {
		constexpr Color WeakSide = Color(StrongSide).getOpposite();

		switch (strongMat + weakMat) {
		case 3: return true; // King and a minor piece against a bare king
		case 6: // King and 2 minor pieces versus a bare king or king and a minor piece versus king and a minor piece
			if (strongMat == 3) { // King and a minor piece versus king and a minor piece
				return true;
			} else { // King and 2 minor pieces versus a bare king
				if (board.bishops(StrongSide) == BitBoard::EMPTY) { // KNNK since there are no bishops
					return true;
				} else if (board.hasOnlySameColoredBishops(StrongSide)) {
					return true; // King and same-colored bishops versus a bare king
				} else {
					return false;
				}
			}
		case 9: // Three minor pieces on the board
			if (strongMat == 6) { // King and 2 minor pieces versus a king and a minor piece
				// 2 bishops versus a bishop or 2 minors but bishop pair versus a knight is a draw
				// 2 same-colored bishops also cannot lead to a win
				if (board.knights(StrongSide) != BitBoard::EMPTY 
					|| board.bishops(WeakSide) == BitBoard::EMPTY 
					|| board.hasOnlySameColoredBishops(StrongSide)) {
					return true;
				} else {
					return false;
				}
			} else {
				return false;
			}
		default: return false;
		}
	}

	// Checks if the current position is drawish
	CM_PURE bool isDrawishEndgame(Board& board) {
		const u8 wMat = board.materialByColor(Color::WHITE);
		const u8 bMat = board.materialByColor(Color::BLACK);
		if (wMat + bMat > 9) { // Does not consider too complex endgames
			return false;
		}

		const bool hasWPawns = board.byPiece(Piece::PAWN_WHITE) != BitBoard::EMPTY;
		const bool hasBPawns = board.byPiece(Piece::PAWN_BLACK) != BitBoard::EMPTY;
		if (hasWPawns || hasBPawns) { // Does not consider endgames with pawns
			return false;
		}

		return wMat > bMat 
			? isDrawishEndgame<Color::WHITE>(board, wMat, bMat) 
			: isDrawishEndgame<Color::BLACK>(board, bMat, wMat);
	}

	// Endgame with king, knight, bishop versus bare king
	template<Color::Value StrongSide>
	CM_PURE Value evalKBNK(Board& board) {
		const Square enemyKing = board.king(Color(StrongSide).getOpposite());
		const u8 kingKingTropism = Square::distance(enemyKing, board.king(StrongSide));

		if (board.byPiece(Piece(StrongSide, PieceType::BISHOP)).b_and(BitBoard::fromColor(Color::WHITE))) {
			constexpr Square corner1 = Square::A8;
			constexpr Square corner2 = Square::H1;

			return kingKingTropism - std::min(Square::distance(corner1, enemyKing), Square::distance(corner2, enemyKing)) * 5;
		} else {
			constexpr Square corner1 = Square::H8;
			constexpr Square corner2 = Square::A1;

			return kingKingTropism - std::min(Square::distance(corner1, enemyKing), Square::distance(corner2, enemyKing)) * 5;
		}
	}

	// Evaluation in case when there is a bare king versus some pieces and enemy king
	// Returns evaluation from the moving side POV
	CM_PURE Value evalSoleKingXPieces(Board& board) {
		Value result = 0;
		
		if (board.materialByColor(Color::WHITE) == 0) { // Bare white king
			if (board.materialByColor(Color::BLACK) == 6 && board.byPiece(Piece::BISHOP_BLACK) && board.byPiece(Piece::KNIGHT_BLACK)) {
				result = -SURE_WIN + evalKBNK<Color::BLACK>(board);
			} else {
				result = -scores::KING_PUSH_TO_CORNER[board.king(Color::WHITE)] - SURE_WIN;
			}
		} else {
			if (board.materialByColor(Color::WHITE) == 6 && board.byPiece(Piece::BISHOP_WHITE) && board.byPiece(Piece::KNIGHT_WHITE)) {
				result = SURE_WIN - evalKBNK<Color::WHITE>(board);
			} else {
				result = scores::KING_PUSH_TO_CORNER[board.king(Color::BLACK)] + SURE_WIN;
			}
		}

		return (-1 + 2 * (board.side() == Color::WHITE)) * result;
	}

	// Evaluation by side for the endgame with pawns and kings only
	template<Color::Value Side>
	CM_PURE Value evalPawnEndgame(Board& board) {
		constexpr Color::Value OppositeSide = Color(Side).getOpposite().value();

		Value result = board.scoreByColor(Side).endgame();
		const Square enemyKingSq = board.king(OppositeSide);
		const Square ourKingSq = board.king(Side);

		const PawnHashEntry& entry = PawnHashTable::getOrScanPHE(board);

		// Everything related purely to pawns is pre-evaluated
		result += entry.pawnEvaluation[Side].endgame();

		// Passed
		BitBoard pawns = entry.pawns[Side];
		BitBoard passed = entry.passed.b_and(pawns);
		BB_FOR_EACH(sq, pawns) {
			if (passed.test(sq)) {
				// Rule of the square
				const Square promotionSq = Square(sq.getFile(), Rank::makeRelativeRank(Side, Rank::R8));
				const bool isEnemySideToMove = board.side() != Side;
				if (std::min(u8(5), Square::distance(sq, promotionSq)) < (Square::distance(enemyKingSq, promotionSq) - isEnemySideToMove)) {
					result += scores::SQUARE_RULE_PASSED;
				}

				// King passed tropism
				result += scores::KING_PASSED_TROPISM * Square::manhattanClosedness(ourKingSq, sq);
				result -= scores::KING_PASSED_TROPISM * Square::manhattanClosedness(enemyKingSq, sq);
			} else {
				// King pawn tropism
				result += scores::KING_PAWN_TROPISM * Square::manhattanClosedness(ourKingSq, sq);
				result -= scores::KING_PAWN_TROPISM * Square::manhattanClosedness(enemyKingSq, sq);
			}
		}

		return result;
	}

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


		///  ENDGAMES  ///

		if (!board.hasNonPawns(Color::WHITE) && !board.hasNonPawns(Color::BLACK)) { // Pawn endgame
			Value result = evalPawnEndgame<Color::WHITE>(board) - evalPawnEndgame<Color::BLACK>(board);
			result *= (-1 + 2 * (board.side() == Color::WHITE));

			return result + scores::TEMPO_SCORE.endgame();
		} else if (isDrawishEndgame(board)) { // Drawish endgame
			return 0;
		} else if (board.materialByColor(Color::WHITE) == 0 || board.materialByColor(Color::BLACK) == 0) { // KXK
			return evalSoleKingXPieces(board);
		} 


		// General evaluation
		Score score = evalSide<Color::WHITE>(board) - evalSide<Color::BLACK>(board);

		///  RESULTS  ///

		Material material = board.materialByColor(Color::WHITE) + board.materialByColor(Color::BLACK);
		Value result = score.collapse(material);
		result *= (-1 + 2 * (board.side() == Color::WHITE));

		return result + scores::TEMPO_SCORE.collapse(material);
	}
}