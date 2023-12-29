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
#include "Chess/Board.h"

/*
*	Tuning(.h/.cpp) contains the functions to tune the evaluation function' weights.
* 
*	Currently it has no automated tuning.
*/

namespace engine {
	class Tuning final {
	public:
		// A single position from a file
		struct Position {
			Board board;
			float result; // Either of 0.0, 0.5, or 1.0
		};

	private:
		std::vector<Position> m_positions;

	public:
		// Extracts a set of positions from the given pgn file
		// <pgnFileName> has no supposed extension, so it must be given explicitly
		// It works not with a true pgn, but rather with a pgn where moves were translated into long algebraic form
		static void extractPositions(const std::string& pgnFileName, const std::string& positionsFileName = "test_suit.fen");

		// Loads an epd file with: fen, res (result)
		void loadPositions(const std::string& fileName);

		// Tries to optimize the given scores by minimizing the error with coordinate descent
		void optimizeScores(const std::vector<Value*>& scores, u32 iterationsCount);

		// Computes the mean error with the current evaluation function
		double computeErr();
	};
}