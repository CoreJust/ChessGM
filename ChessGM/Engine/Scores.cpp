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

#include "Scores.h"

#define Z Score()
#define S(mg, eg) Score(mg, eg)

namespace scores {
	Score TEMPO_SCORE = S(15, 3);


	///  PAWNS  ///

	// [number of pawn islands]
	// A pawn island is a series of pawns each on the next file to the previous
	Score PAWN_ISLANDS[5] = { Z, Z, S(-3, -3), S(-21, -16), S(-34, -32) };

	// A pawn that is defended by another pawn.
	//Score DEFENDED_PAWN = S(12, 12);
	Score DEFENDED_PAWN[Rank::VALUES_COUNT] = { Z, Z, S(3, 5), S(7, 8), S(13, 15), S(19, 23), S(28, 36), Z };

	// Duo pawns are neigbbouring pawns on the same rank
	Score ISOLATED_PAWN = S(-7, -5);

	// Pawn that cannot be protected by own pawns and cannot safely advance
	Score BACKWARD_PAWN = S(-9, -9);

	// Several pawns on the same file
	Score DOUBLE_PAWN = S(-10, -23);

	// Pawn distortion is how far are the pawns on the adjacent files from each other
	Score PAWN_DISTORTION = S(-1, -2);

	// Bonus for a passed pawn depending on its advancement
	Score PASSED_PAWN[Rank::VALUES_COUNT] = {
		Z, S(15, 25), S(22, 30), S(30, 35), S(42, 48), S(55, 65), S(75, 95), Z
	};

	// A rook that supports the passed from behind
	Score ROOK_BEHIND_PASSED_PAWN = S(12, 28);

	// A passed is blocked with a minor piece
	Score MINOR_PASSED_BLOCKED = S(-14, -27);


	///  MINOR PIECES  ///

	// Bonus for outpost for any minor piece, with multiplicator for knight
	Score OUTPOST = S(18, 10);


	///  KNIGHT  ///

	// Bonus depending on number of possible moves
	Score KNIGHT_MOBILITY[9] = {
		S(-90, -120), S(-35, -60), S(-16, -25), S(-5, -10), S(5, 3), S(14, 11), S(21, 17), S(25, 21), S(28, 24)
	};


	///  BISHOP  ///

	// Bonus depending on number of possible moves
	Score BISHOP_MOBILITY[14] = {
		S(-60, -90), S(-35, -50), S(-20, -25), S(-10, -14), S(-5, -8), S(0, -3), S(6, 4), S(12, 11), S(18, 18), S(25, 25), 
		S(30, 30), S(35, 35), S(40, 40), S(45, 45)
	};

	// Bonus for a pair of different-colored bishops
	Score BISHOP_PAIR = S(35, 20);

	
	///  ROOK  ///

	// Bonus depending on number of possible moves
	Score ROOK_MOBILITY[15] = {
		S(-45, -70), S(-30, -45), S(-18, -24), S(-10, -14), S(-5, -8), S(0, -2), S(6, 5), S(12, 12), S(18, 20), S(25, 27),
		S(30, 34), S(35, 41), S(40, 48), S(45, 55), S(50, 62)
	};

	// Bonus for a rook positioned on a file with no pawns
	Score ROOK_ON_OPEN_FILE = S(26, 10);

	// Bonus for a rook positioned on a file without own pawns, but with enemy pawn
	Score ROOK_ON_SEMIOPEN_FILE = S(14, 6);


	///  QUEEN  ///

	// Bonus depending on number of possible moves
	Score QUEEN_MOBILITY[28] = {
		S(-35, -55), S(-28, -37), S(-22, -26), S(-17, -19), S(-12, -13), S(-8, -8), S(-4, -4), S(0, 1), S(4, 5), S(8, 10),
		S(12, 14), S(16, 19), S(20, 23), S(24, 28), S(28, 32), S(32, 37), S(36, 41), S(40, 46), S(44, 50), S(47, 54),
		S(50, 57), S(53, 61), S(56, 64), S(59, 67), S(62, 71), S(65, 74), S(67, 76), S(70, 80)
	};


	///  KPsKPs  ///

	// Square rule is when a passed cannot be reached by the enemy king
	Value SQUARE_RULE_PASSED = 200;

	// Bonus factor for king being near a passed in pawn endgame
	Value KING_PASSED_TROPISM = 5;

	// Bonus factor for king being close to pawns in pawn endgame
	Value KING_PAWN_TROPISM = 2;


	///  TABLES  ///

	// Table for evaluation in endagames with a sole king on one side
	Value KING_PUSH_TO_CORNER[Square::VALUES_COUNT] = {
		100,90,	80,	70,	70,	80,	90,	100,
		90,	60,	50,	40,	40,	50,	60,	90,
		80,	50,	30,	20,	20,	30,	50,	80,
		70,	40,	20,	10,	10,	20,	40,	70,
		70,	40,	20,	10,	10,	20,	40,	70,
		80,	50,	30,	20,	20,	30,	50,	80,
		90,	60,	50,	40,	40,	50,	60,	90,
		100,90,	80,	70,	70,	80,	90,	100
	};

	// Pieces' cost
	Score PIECE_VALUE[PieceType::VALUES_COUNT] = {
		Z,	// None
		S(100, 130), // Pawn
		S(320, 360), // Knight
		S(350, 390), // Bishop
		S(550, 650), // Rook
		S(1050, 1150), // Queen
		Z  // King
	};

	// Piece-square tables
	Score PST[Piece::VALUES_COUNT][Square::VALUES_COUNT] = {
		{ Z }, { Z }, // None
		{ }, { // Pawn
			 Z,				Z,				Z,				Z,
			 S(15,  32),	S(20,  45),		S(16,  45),		S(25,  45),
			 S(7,   20),	S(10,  25),		S(10,  25),		S(18,  25),
			 S(0,   10),	S(2,   15),		S(6,   15),		S(15,  15),
			 S(-4,  5),		S(0,   10),		S(4,   10),		S(12,  10),
			 S(-1,  0),		S(4,   5),		S(-4,  5),		S(0,   5),
			 S(-6,  -5),	S(-3,  0),		S(4,   0),		S(-12, 0),
			 Z,				Z,				Z,				Z,
		}, { }, { // Knight
			 S(-65,	 -40),	S(-40,	 -20),	S(-22,	 -20),	S(-15,	-15),
			 S(-45,	 -30),	S(-15,	 -9),	S(7,	 2),	S(10,	5),
			 S(-20,	 -14),	S(3,	 2),	S(15,	 10),	S(26,	17),
			 S(-12,	 -8),	S(10,	 5),	S(24,	 15),	S(40,	23),
			 S(-15,	 -10),	S(5,	 5),	S(20,	 15),	S(36,	23),
			 S(-30,	 -20),	S(0,	 2),	S(12,	 10),	S(23,	17),
			 S(-45,	 -30),	S(-16,	 -9),	S(2,	 2),	S(8,	5),
			 S(-60,	 -40),	S(-25,	 -20),	S(-22,	 -20),	S(-25,	-15),
		}, { }, { // Bishop
			 S(-15,	 -20),	S(-14,	 -15),	S(-9,	 -10),	S(-15,	-10),
			 S(-10,	 -15),	S(5,	 10),	S(2,	 5),	S(-2,	0),
			 S(-5,	 -10),	S(7,	 5),	S(5,	 10),	S(8,	5),
			 S(0,	 -10),	S(-5,	 0),	S(10,	 5),	S(15,	10),
			 S(0,	 -10),	S(-5,	 0),	S(10,	 5),	S(15,	10),
			 S(10,	 -10),	S(5,	 5),	S(5,	 10),	S(9,	5),
			 S(5,	 -15),	S(20,	 10),	S(3,	 5),	S(0,	0),
			 S(-5,	 -20),	S(-12,	 -15),	S(1,	 -10),	S(-10,	-10),
		}, { }, { // Rook
			 S(-12,	 -1),	S(-10,	 0),	S(-4,	 0),	S(-1,	0),
			 S(-8,	 0),	S(4,	 0),	S(5,	 0),	S(5,	0),
			 S(-15,	 0),	S(-2,	 0),	S(-5,	 0),	S(-5,	0),
			 S(-20,	 0),	S(-5,	 0),	S(-10,	 0),	S(-20,	0),
			 S(-20,	 0),	S(-5,	 0),	S(-10,	 0),	S(-20,	0),
			 S(-15,	 0),	S(-2,	 0),	S(-5,	 0),	S(-5,	0),
			 S(-8,	 0),	S(0,	 0),	S(1,	 0),	S(12,	0),
			 S(-10,	 -1),	S(-8,	 0),	S(2,	 0),	S(20,	0),
		}, { }, { // Queen
			 S(-8,	 -20),	S(-10,	 -15),	S(-10,	 -10),	S(0,	-5),
			 S(0,	 -15),	S(0,	 -9),	S(0,	 0),	S(10,	0),
			 S(0,	 -10),	S(0,	 0),	S(0,	 5),	S(6,	6),
			 S(0,	 -5),	S(0,	 3),	S(4,	 10),	S(3,	12),
			 S(0,	 -5),	S(0,	 3),	S(4,	 10),	S(4,	12),
			 S(0,	 -10),	S(0,	 0),	S(0,	 5),	S(0,	6),
			 S(0,	 -15),	S(0,	 -9),	S(0,	 0),	S(0,	0),
			 S(-8,	 -20),	S(-8,	 -15),	S(-5,	 -10),	S(0,	-5),
		}, { }, { // King
			 S(-70,	 -60),	S(-70,	 -45),	S(-75,	 -40),	S(-80,	-35),
			 S(-80,	 -45),	S(-80,	 -25),	S(-85,	 -20),	S(-85,	-15),
			 S(-80,	 -40),	S(-80,	 -20),	S(-85,	 -5),	S(-85,	0),
			 S(-70,	 -35),	S(-70,	 -15),	S(-70,	 0),	S(-70,	10),
			 S(-55,	 -35),	S(-55,	 -15),	S(-60,	 0),	S(-65,	10),
			 S(-40,	 -40),	S(-45,	 -20),	S(-45,	 -5),	S(-50,	0),
			 S(-5,	 -45),	S(-5,	 -25),	S(-25,	 -20),	S(-30,	-15),
			 S(25,	 -60),	S(35,	 -45),	S(7,	 -40),	S(-5,	-35),
		}
	};

	Value SIMPLIFIED_PIECE_VALUES[Piece::VALUES_COUNT];

	void initScores() {
		// Simplified piece values
		for (Piece piece : Piece::iter()) {
			const Score score = PIECE_VALUE[piece.getType()];
			SIMPLIFIED_PIECE_VALUES[piece] = (score.middlegame() + score.endgame()) / 2;
		}

		// PST
		Score tmp[32];
		for (auto pt : PieceType::iter()) {
			memcpy(tmp, PST[Piece(Color::WHITE, pt)], 32 * sizeof(Score));
			for (u8 i = 0; i < 32; ++i) {
				const Rank rank = Rank(i >> 2);
				const File file = File(i & 3);
				const Square sqB = Square(file, rank); // For black
				const Square sqW = sqB.getOpposite(); // For white

				const Score score = tmp[i] + PIECE_VALUE[pt];

				PST[Piece(Color::WHITE, pt)][sqW] = PST[Piece(Color::WHITE, pt)][sqW.mirrorByFile()] = score;
				PST[Piece(Color::BLACK, pt)][sqB] = PST[Piece(Color::BLACK, pt)][sqB.mirrorByFile()] = score;
			}
		}
	}
}