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
#include "Chess/Score.h"
#include "Chess/Defs.h"

/*
*	Scores(.h/.cpp) contains the weights for the evaluation function.
*/

namespace scores {
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