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

#include "ConsoleColor.h"

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#else
#include <term.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif // _WIN32

std::ostream& io::operator<<(std::ostream& out, const ConsoleColor color) {
	if (out.rdbuf() != std::cout.rdbuf()) { // For std::cout only
		return out;
	}

#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)((uint8_t(color.back) << 4) | uint8_t(color.front)));
#else // Linux
	std::cout << L"\033[%d;%dm" << (uint8_t(color.front) + 30) << (uint8_t(color.back) + 40);
#endif

	return out;
}

std::ostream& io::operator<<(std::ostream& out, const Color color) {
	return out << ConsoleColor { .back = Color::Black, .front = color };
}
