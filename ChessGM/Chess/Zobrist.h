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
#include "Defs.h"

/*
*	Zobrist(.h/.cpp) contains the Zobrist key for position hashing.
*/

namespace zobrist {
	extern const Hash PIECE[Piece::VALUES_COUNT][Square::VALUES_COUNT];
	extern const Hash SIDE[Color::VALUES_COUNT];
	extern const Hash EP[File::VALUES_COUNT];
	extern const Hash CASTLING[64];

	inline constexpr Hash MOVE_KEY = 0x54ca3eb5b5f3cb5bull;
	inline constexpr Hash NULL_MOVE_KEY = 0x08d9bc25bebf91b1ull;
}