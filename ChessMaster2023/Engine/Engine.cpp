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

#include "Engine.h"

namespace engine {
	Board g_board;
	std::vector<Move> g_moveHistory; 
	std::string g_errorMessage;

	void run(io::IOMode mode) {
		bool (*handle)(std::string, const std::vector<std::string>&)
			= mode == io::CONSOLE
				? handleConsole
			: mode == io::UCI
				? handleUCI
				: handleXboard;

		if (mode == io::CONSOLE) {
			g_board = Board::makeInitialPosition(); // Set the default position right away in console mode
			options::g_forceMode = true;
		} else if (mode == io::UCI) {
			options::g_forceMode = true;
		}

		// Get another command in a loop and handle it in the current mode.
		std::vector<std::string> args;
		std::string cmd;

		do {
			cmd = io::getCommand(args);
		} while (handle(std::move(cmd), args));
	}

	void checkInput() {
		if (!io::hasInput()) { // Has no input
			return;
		}

		void (*check)(std::string, const std::vector<std::string>&)
			= io::getMode() == io::CONSOLE
				? checkConsole
			: io::getMode() == io::UCI
				? checkUCI
				: checkXboard;

		std::vector<std::string> args;
		std::string cmd = io::getCommand(args, false);
		check(std::move(cmd), args);
	}

	bool newGame(std::string_view fen) {
		bool success;
		g_board = Board::fromFEN(fen, success);
		g_moveHistory.clear();

		if (!success) {
			g_errorMessage = fen;
			return false;
		}

		return true;
	}

	bool makeMove(std::string_view move) {
		Move m = g_board.makeMoveFromString(move);
		if (m.isNullMove()) {
			g_errorMessage = move;
			return false;
		}

		g_board.makeMove(m);
		g_moveHistory.push_back(m);
		return true;
	}

	bool unmakeMove() {
		if (g_moveHistory.empty()) {
			g_errorMessage = "No move to unmake";
			return false;
		}

		g_board.unmakeMove(g_moveHistory.back());
		g_moveHistory.pop_back();
		return true;
	}
}