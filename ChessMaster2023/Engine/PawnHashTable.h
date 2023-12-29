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
*	PawnHashTable(.h/.cpp) contains the hash tables for pawns.
* 
*	It is a hash table with small size but large elements that stores the information on
*	pawn structure and accelerates the evaluation.
*/

namespace engine {
	// Contains all the required information on pawns for a position
	struct PawnHashEntry final {
		BitBoard pawns[Color::VALUES_COUNT]; // For verification
		BitBoard passed;
		BitBoard isolated;
		BitBoard doubled;
		BitBoard backward;
		Rank mostAdvanced[Color::VALUES_COUNT][Rank::VALUES_COUNT + 2]; // +2 so that we can check the adjacent files for any file on the board
		Score pawnEvaluation[Color::VALUES_COUNT];
		u8 islandsCount[Color::VALUES_COUNT];
		u8 distortion[Color::VALUES_COUNT];
	};

	// Contains the table of PawnHashEntry's
	class PawnHashTable final {
	public:
		constexpr inline static uint32_t PAWN_HASH_TABLE_SIZE_LOG2 = 12; // Nodes number in the table = 2**PAWN_HASH_TABLE_SIZE_LOG2

	private:
		static PawnHashEntry s_table[1 << PAWN_HASH_TABLE_SIZE_LOG2];

	public:
		static void init();
		static void reset();

		// Returns an entry from the table if there is, or creates a new one
		static PawnHashEntry& getOrScanPHE(Board& board);

	private:
		template<Color::Value Side>
		static void scanPawns(Board& board, PawnHashEntry& entry);
	};
}