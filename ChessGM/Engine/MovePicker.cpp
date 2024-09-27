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

#include "MovePicker.h"

namespace engine {
	uint32_t s_historyTries[Piece::VALUES_COUNT][Square::VALUES_COUNT];
	uint32_t s_historySuccesses[Piece::VALUES_COUNT][Square::VALUES_COUNT];

	SearchStack MovePicker::s_noSS { 
		.firstKiller = Move::makeNullMove(), 
		.secondKiller = Move::makeNullMove() 
	};
}