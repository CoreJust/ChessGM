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

#include "Eval.h"
#include "PawnHashTable.h"

namespace engine {
	const BitBoard OUTPOSTS_BB[Color::VALUES_COUNT] = {
		// Black
		BitBoard::fromRank(Rank::R5).b_or(BitBoard::fromRank(Rank::R4)).b_or(BitBoard::fromRank(Rank::R3))
			.b_and(BitBoard::fromFile(File::A).b_or(BitBoard::fromFile(File::H)).b_not()),

		// White
		BitBoard::fromRank(Rank::R4).b_or(BitBoard::fromRank(Rank::R5)).b_or(BitBoard::fromRank(Rank::R6))
			.b_and(BitBoard::fromFile(File::A).b_or(BitBoard::fromFile(File::H)).b_not())
	};

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
	CM_PURE Score evalSide(Board& board, const PawnHashEntry& entry) {
		constexpr Color::Value OppositeSide = Color(Side).getOpposite().value();
		constexpr Direction::Value Up = Direction::makeRelativeDirection(Side, Direction::UP).value();
		constexpr Direction::Value Down = Direction::makeRelativeDirection(Side, Direction::DOWN).value();
		constexpr Rank Rank1 = Rank::makeRelativeRank(Side, Rank::R1);
		constexpr Rank Rank8 = Rank::makeRelativeRank(Side, Rank::R8);


		Score result = board.scoreByColor(Side);
		const BitBoard ourPieces = board.byColor(Side);
		const BitBoard occ = ourPieces.b_or(board.byColor(OppositeSide));

		const BitBoard ourPawnsAttacks = entry.pawns[Side].pawnAttackedSquares<Side>();
		const BitBoard enemyPawnsAttacks = entry.pawns[OppositeSide].pawnAttackedSquares<OppositeSide>();
		const BitBoard attackableSquares = ourPieces.b_or(enemyPawnsAttacks).b_not(); // Squares accounted when evaluating mobility
		const BitBoard outpostSquares = OUTPOSTS_BB[Side].b_and(ourPawnsAttacks);


		///  PAWNS   ///
		
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


		///  KNIGHTS  ///

		pieces = board.knights(Side);
		BB_FOR_EACH(sq, pieces) {
			BitBoard attacks = BitBoard::pseudoAttacks<PieceType::KNIGHT>(sq).b_and(attackableSquares);

			// Mobility
			result += scores::KNIGHT_MOBILITY[attacks.popcnt()];

			// Outpost
			if (outpostSquares.test(sq) && BitBoard::directionBits<Up>(sq).b_and(enemyPawnsAttacks) == BitBoard::EMPTY) {
				result += scores::OUTPOST * 2;
			}
		}


		///  BISHOPS  ///

		// Bishop pair
		if (board.hasDifferentColoredBishops(Side)) {
			result += scores::BISHOP_PAIR;
		}

		pieces = board.bishops(Side);
		BB_FOR_EACH(sq, pieces) {
			BitBoard attacks = BitBoard::attacksOf(PieceType::BISHOP, sq, occ).b_and(attackableSquares);

			// Mobility
			result += scores::BISHOP_MOBILITY[attacks.popcnt()];

			// Outpost
			if (outpostSquares.test(sq) && BitBoard::directionBits<Up>(sq).b_and(enemyPawnsAttacks) == BitBoard::EMPTY) {
				result += scores::OUTPOST;
			}
		}


		///  ROOKS  ///

		pieces = board.rooks(Side);
		BB_FOR_EACH(sq, pieces) {
			BitBoard attacks = BitBoard::attacksOf(PieceType::ROOK, sq, occ).b_and(attackableSquares);

			// Mobility
			result += scores::ROOK_MOBILITY[attacks.popcnt()];

			// Rook on (semi)open file
			if (entry.mostAdvanced[Side][sq.getFile() + 1] == Rank1) { // No our pawns on the file
				if (entry.mostAdvanced[OppositeSide][sq.getFile() + 1] == Rank8) { // No enemy pawns as well
					result += scores::ROOK_ON_OPEN_FILE;
				} else {
					result += scores::ROOK_ON_SEMIOPEN_FILE;
				}
			}
		}


		///  QUEEN  ///

		pieces = board.queens(Side);
		BB_FOR_EACH(sq, pieces) {
			BitBoard attacks = BitBoard::attacksOf(PieceType::QUEEN, sq, occ).b_and(attackableSquares);

			// Mobility
			result += scores::QUEEN_MOBILITY[attacks.popcnt()];
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
		const PawnHashEntry& entry = PawnHashTable::getOrScanPHE(board);
		Score score = evalSide<Color::WHITE>(board, entry) - evalSide<Color::BLACK>(board, entry);


		///  RESULTS  ///

		Material material = board.materialByColor(Color::WHITE) + board.materialByColor(Color::BLACK);
		Value result = score.collapse(material);
		result *= (-1 + 2 * (board.side() == Color::WHITE));

		return result + scores::TEMPO_SCORE.collapse(material);
	}
}