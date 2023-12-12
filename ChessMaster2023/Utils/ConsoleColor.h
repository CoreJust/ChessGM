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
#include <iostream>

/*
*	ConsoleColor(.h/.cpp) contains utilities to make colored text in console.
*/

namespace io {
#ifdef _WIN32
	enum class Color : uint8_t {
		Black = 0,
		Blue = 1,
		Green = 2,
		Cyan = 3,
		Red = 4,
		Magenta = 5,
		Yellow = 14,
		White = 15
	};
#else // Linux
	enum class Color : uint8_t {
		Black = 0,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan,
		White
	};
#endif

	struct ConsoleColor {
		Color back = Color::Black;
		Color front = Color::White;
	};

	std::ostream& operator<<(std::ostream& out, const ConsoleColor color);
	std::ostream& operator<<(std::ostream& out, const Color color);
}