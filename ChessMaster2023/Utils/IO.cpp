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

#include "IO.h"
#include <cassert>
#include <queue>

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#else
#include <term.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif // _WIN32

#include "ChessMasterInfo.h"
#include "StringUtils.h"

///  GLOBAL VARIABLES  ///

io::Output io::g_out;
std::ofstream io::Output::s_logFile;

io::IOMode g_mode;
u32 g_xboardVersion; // For Xboard mode only
std::string g_cmd; // For commands
std::string g_allArguments;


#ifdef _WIN32
HANDLE g_handle = 0;
#endif

bool g_isPipe = false;

// The commands that were read but not processed right away
std::queue<std::pair<std::string, std::vector<std::string>>> g_queuedCommands;


///  FUNCTIONS   ///

void initForConsole() {
	io::g_out << io::Color::Green
		<< "Welcome to the "
		<< io::Color::Blue << ENGINE_NAME << " " << ENGINE_VERSION << io::Color::Green
		<< " chess engine by " << io::Color::Blue << AUTHOR_NAME << io::Color::Green << "!" <<
		"\nYou are currently in the console mode."\
		"\nThis engine supports UCI and Xboard/Winboard, so you can run it in GUI."\
		"\nIn the current console mode, there is a specific console interface."\
		"\nTo get the commands available now, type help or h"
		<< io::Color::White << std::endl;
}

void initForXboard() {
	// Require xboard version 2 or higher
	std::vector<std::string> args;
	std::string cmd = io::getCommand(args);

	if (cmd != "protover") {
		exit(1);
	}

	g_xboardVersion = str_utils::fromString<u32>(args[0]);

	io::g_out << "feature ping=1, setboard=1, playother=0, san=0, usermove=1, time=1, draw=1, reuse=1, analyze=1, myname=\""
		<< ENGINE_NAME << " " << ENGINE_VERSION << " by " << AUTHOR_NAME << "\"" << std::endl
		<< "feature variants=\"normal\"" << std::endl
		<< "feature ics=1, name=1, pause=1, colors=0, nps=1, done=1" << std::endl;
}

void initForUCI() {
	io::g_out << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << std::endl
		<< "id author " << AUTHOR_NAME << std::endl;
	io::g_out << "uciok" << std::endl;
}

io::IOMode requestIOMode() {
	auto cmd = io::getLine();

	if (cmd == "uci") {
		return io::IOMode::UCI;
	} else if (cmd == "xboard") {
		return io::IOMode::XBOARD;
	} else {
		return io::IOMode::CONSOLE;
	}
}

void io::Output::init() {
	s_logFile.open("log.txt");
	s_logFile << std::unitbuf;

	assert(s_logFile.is_open() && s_logFile.good());
}

void io::Output::destroy() {
	s_logFile.close();
}

void io::Output::logInput(std::string_view str) {
	if (options::g_debugMode) {
		s_logFile << "Input: " << str << std::endl;
	}
}

io::Output& io::Output::operator<<(std::ostream& (__cdecl* func)(std::ostream&)) {
	if (options::g_debugMode) {
		s_logFile << func;
	}

	if (getMode() == IOMode::CONSOLE) {
		std::cout << Color::White;
	}

	std::cout << func;
	return *this;
}

void io::init() {
	// OS-specific initialization
#ifdef _WIN32
	DWORD _;
	g_handle = GetStdHandle(STD_INPUT_HANDLE);
	g_isPipe = !GetConsoleMode(g_handle, &_);
#else
	g_isPipe = !isatty(0);
	if (g_isPipe) {
		signal(SIGINT, SIG_IGN);
	}
#endif

	// General initialization
	std::cout.setf(std::ios::unitbuf);
	std::ios_base::sync_with_stdio(false);

	g_mode = requestIOMode();
	switch (g_mode) {
		case io::CONSOLE: initForConsole(); break;
		case io::XBOARD: initForXboard(); break;
		case io::UCI: initForUCI(); break;
	default: break;
	}
}

void io::pushCommand(std::string cmd, std::vector<std::string> args) {
	g_queuedCommands.push(std::make_pair<std::string, std::vector<std::string>>(std::move(cmd), std::move(args)));
}

bool io::hasCommandsInQueue() {
	return !g_queuedCommands.empty();
}

std::string_view io::getLine() {
	std::getline(std::cin, g_cmd);
	Output::logInput(g_cmd);

	return g_cmd;
}

std::string io::getCommand(std::vector<std::string>& args, const bool lookInQueue) {
	if (lookInQueue && !g_queuedCommands.empty()) { // We have some unhandled commands
		auto [cmd, _args] = std::move(g_queuedCommands.front());
		g_queuedCommands.pop();

		args = std::move(_args);
		return cmd;
	}

	if (g_mode == IOMode::CONSOLE) {
		std::cout << ">>> ";
	}

	getLine();
	size_t i = 0;

	args.clear();

	// Command
	while (i < g_cmd.size() && !isspace(g_cmd[i])) i++;
	std::string cmd = g_cmd.substr(0, i);
	g_allArguments = "";

	// Check if the command has no arguments
	if (i >= g_cmd.size()) {
		return cmd;
	}

	// Arguments
	size_t from = i; // The character after the previous whitespace
	while (i < g_cmd.size()) {
		// Skipping whitespaces
		while (i < g_cmd.size() && isspace(g_cmd[i])) from = ++i;
		while (i < g_cmd.size() && !isspace(g_cmd[i])) ++i;

		if (i != from) {
			args.emplace_back(g_cmd.substr(from, i - from));
		}
	}

	g_allArguments = g_cmd.substr(cmd.size() + 1);
	return cmd;
}

std::string_view io::getAllArguments() noexcept {
	return g_allArguments;
}

io::IOMode io::getMode() {
	return g_mode;
}

u32 io::getXboardVersion() {
	assert(g_mode == IOMode::XBOARD);

	return g_xboardVersion;
}

bool io::hasInput() {
#ifdef _WIN32
	if (g_isPipe) {
		DWORD charsNumber = 0;
		return PeekNamedPipe(g_handle, nullptr, 0, nullptr, &charsNumber, nullptr) ? bool(charsNumber) : true;
	} else {
		return bool(_kbhit());
	}
#else // Found it somewhere once since I am not familiar with linux system functions
	static fd_set input_fd_set;
	static fd_set except_fd_set;

	FD_ZERO(&input_fd_set);
	FD_ZERO(&except_fd_set);
	FD_SET(0, &input_fd_set);
	FD_SET(1, &except_fd_set);

	static timeval timeout;
	static int max_fd = 2;

	timeout.tv_sec = timeout.tv_usec = 0;
	if (select(max_fd, &input_fd_set, NULL, &except_fd_set, &timeout) <= 0) {
		return false;
	}

	return FD_ISSET(0, &input_fd_set);
#endif
}