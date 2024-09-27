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

#pragma once
#include "Chess/Score.h"
#include "Chess/Defs.h"

/*
*	Scores(.h/.cpp) contains the weights for the evaluation function.
*/

namespace scores {
	// Scores
	extern Score PAWN_ISLANDS[5];
	extern Score DEFENDED_PAWN[Rank::VALUES_COUNT];
	extern Score ISOLATED_PAWN;
	extern Score BACKWARD_PAWN;
	extern Score DOUBLE_PAWN;
	extern Score PAWN_DISTORTION;

	extern Score PASSED_PAWN[Rank::VALUES_COUNT];
	extern Score ROOK_BEHIND_PASSED_PAWN;
	extern Score MINOR_PASSED_BLOCKED;

	extern Score NO_PAWNS;

	extern Score OUTPOST;

	extern Score KNIGHT_MOBILITY[9];

	extern Score BISHOP_MOBILITY[14];
	extern Score BISHOP_PAIR;

	extern Score ROOK_MOBILITY[15];
	extern Score ROOK_ON_OPEN_FILE;
	extern Score ROOK_ON_SEMIOPEN_FILE;

	extern Score QUEEN_MOBILITY[28];

	extern Value SQUARE_RULE_PASSED;
	extern Value KING_PASSED_TROPISM;
	extern Value KING_PAWN_TROPISM;

	extern Value KING_PUSH_TO_CORNER[Square::VALUES_COUNT];

	extern Score PIECE_VALUE[PieceType::VALUES_COUNT]; // Pieces' cost
	extern Score PST[Piece::VALUES_COUNT][Square::VALUES_COUNT]; // Piece-square tables

	extern Value SIMPLIFIED_PIECE_VALUES[Piece::VALUES_COUNT]; // Simplified always positive piece values for SEE

	extern Score TEMPO_SCORE;

	// TODO: implement
	// Intended to be used for loading/storing the weights
	class Weights final {

	};

	void initScores();
}

// These are some common things that are convenient to put in Scores.h, although they
// do not logically belong here
namespace engine {
	// Some common constants
	constexpr Depth MAX_DEPTH = 99;

	constexpr Value INF = 31000;
	constexpr Value MATE = 30000;
	constexpr Value SURE_WIN = 20000; // A value that cannot be reached with normal evaluation


	///  AUXILIARY FUNCTIONS  ///

	CM_PURE constexpr bool isMateValue(const Value value) noexcept {
		return (value > MATE - MAX_DEPTH * 2 && value <= MATE) || (value < MAX_DEPTH * 2 - MATE && value >= -MATE);
	}

	// Moves before the mate
	CM_PURE constexpr Depth givingMateIn(const Value value) noexcept {
		return (MATE + 2 - value) / 2;
	}

	// Moves before the mate
	CM_PURE constexpr Depth gettingMatedIn(const Value value) noexcept {
		return (value + MATE + 1) / 2;
	}
}