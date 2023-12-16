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

#include "Scores.h"

#define Z Score()
#define S(mg, eg) Score(mg, eg)

namespace scores {
	Score TEMPO_SCORE = S(20, 5);

	// Pieces' cost
	Score PIECE_VALUE[PieceType::VALUES_COUNT] = {
		Z,	// None
		S(100, 130), // Pawn
		S(320, 360), // Knight
		S(350, 360), // Bishop
		S(550, 600), // Rook
		S(1100, 1250), // Queen
		Z  // King
	};

	// Piece-square tables
	Score PST[Piece::VALUES_COUNT][Square::VALUES_COUNT] = {
		{ Z }, { Z }, // None
		{ }, { // Pawn
			 Z,				Z,				Z,				Z,
			 S(20,  40),	S(20,  45),		S(16,  45),		S(20,  45),
			 S(11,  25),	S(10,  25),		S(10,  25),		S(16,  25),
			 S(3,   15),	S(2,   15),		S(6,   15),		S(12,  15),
			 S(0,   10),	S(0,   10),		S(4,   10),		S(10,  10),
			 S(3,   5),		S(4,   5),		S(-4,  5),		S(2,   5),
			 S(-2,  0),		S(-3,  0),		S(4,   0),		S(-10, 0),
			 Z,				Z,				Z,				Z,
		}, { }, { // Knight
			 S(-65,	 -40),	S(-40,	 -20),	S(-22,	 -20),	S(-15,	-15),
			 S(-45,	 -30),	S(-15,	 -9),	S(7,	 2),	S(10,	5),
			 S(-20,	 -14),	S(3,	 2),	S(15,	 10),	S(25,	16),
			 S(-12,	 -8),	S(10,	 5),	S(24,	 15),	S(35,	21),
			 S(-15,	 -10),	S(5,	 5),	S(20,	 15),	S(30,	21),
			 S(-30,	 -20),	S(0,	 2),	S(12,	 10),	S(22,	16),
			 S(-45,	 -30),	S(-16,	 -9),	S(2,	 2),	S(8,	5),
			 S(-60,	 -40),	S(-30,	 -20),	S(-30,	 -20),	S(-25,	-15),
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
			 S(-8,	 0),	S(-2,	 0),	S(-5,	 0),	S(-1,	0),
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
			 S(-70,	 -30),	S(-70,	 -25),	S(-75,	 -20),	S(-80,	-14),
			 S(-80,	 -25),	S(-80,	 -15),	S(-85,	 -5),	S(-85,	-5),
			 S(-80,	 -20),	S(-80,	 -5),	S(-85,	 3),	S(-85,	8),
			 S(-70,	 -14),	S(-70,	 -5),	S(-70,	 8),	S(-70,	15),
			 S(-55,	 -14),	S(-55,	 -5),	S(-60,	 8),	S(-65,	15),
			 S(-40,	 -20),	S(-45,	 -5),	S(-45,	 3),	S(-50,	8),
			 S(-5,	 -25),	S(-5,	 -15),	S(-25,	 -5),	S(-30,	-5),
			 S(25,	 -30),	S(35,	 -25),	S(7,	 -20),	S(-5,	-14),
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