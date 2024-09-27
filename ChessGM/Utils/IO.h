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
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

#include "ConsoleColor.h"
#include "Engine/Options.h"

/*
*	IO(.h/.cpp) contains functions for I/O usage.
* 
*	It initializes I/O, requests the working mode (own console / XBoard / UCI), and then just
*	provides some simple interface for using I/O.
*/

namespace io {
	enum IOMode : uint8_t {
		CONSOLE = 0, // Own console interface
		XBOARD,
		UCI,

		NUMBER_IO_MODES
	};


	///  AUXILIARY CLASS OUTPUT  ///

	// Used to redirect the output
	class Output final {
	private:
		static std::ofstream s_logFile;

	public:
		static void init();
		static void destroy();

		static void logInput(std::string_view str);

		Output& operator<<(auto&& value) {
			if (options::g_debugMode) {
				s_logFile << value;
			}

			std::cout << value;
			return *this;
		}

		Output& operator<<(std::ostream& (__cdecl* func)(std::ostream&));
	};

	extern Output g_out;


	///  IO FUNCTIONS  ///

	void init();

	// Pushes a command into the queue
	void pushCommand(std::string cmd, std::vector<std::string> args);
	bool hasCommandsInQueue();

	std::string_view getLine();

	// Requests a command from console and parses it
	// Returns the command and puts its arguments into <args>
	// If lookInQueue is true, then it first checks if there are any commands in the queue
	// It can return then the first command in queue, otherwise it processes input
	std::string getCommand(std::vector<std::string>& args, const bool lookInQueue = true);

	// Returns the arguments as a single string
	std::string_view getAllArguments() noexcept;
	IOMode getMode();
	u32 getXboardVersion();

	// Checks if there is any input available
	bool hasInput();
}