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
#include "Utils/IO.h"
#include "Chess/Board.h"
#include "Options.h"

/*
*	Engine(.h/.cpp) and EngineConsole.cpp/EngineUCI.cpp/EngineXboard.cpp contain the main loop
*	for each mode. They process the input and manage the whole chess engine.
* 
*	The common functions are implemented in Engine.cpp.
*	The functions with Xboard/Console/UCI in name are implemented in EngineXboard.cpp/EngineConsole.cpp/EngineUCI.cpp.
*/

namespace engine {
	extern Board g_board;
	extern std::vector<Move> g_moveHistory;
	extern std::string g_errorMessage; // It is used to pass an error message from the common engine functions

	// Must return false on quitting
	bool handleXboard(std::string cmd, const std::vector<std::string>& args);
	bool handleConsole(std::string cmd, const std::vector<std::string>& args);
	bool handleUCI(std::string cmd, const std::vector<std::string>& args);

	void checkXboard(std::string cmd, const std::vector<std::string>& args);
	void checkConsole(std::string cmd, const std::vector<std::string>& args);
	void checkUCI(std::string cmd, const std::vector<std::string>& args);

	void run(io::IOMode mode);

	// Must be called once in a while during continuous processes
	// Checks if there is any input available and handles it if necessary
	// This works with very few commands - most are just pushed to the queue
	void checkInput();

	// Common engine functions (implemented in Engine.cpp)
	// Return false on any error, the error would be in g_errorMessage.

	bool newGame(std::string_view fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
	bool makeMove(std::string_view move);
	bool unmakeMove();
}