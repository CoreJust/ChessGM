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
#include "Utils/CommandHandlingUtils.h"
#include "Utils/StringUtils.h"
#include "Eval.h"
#include "Search.h"

namespace engine {
	void handleIncorrectCommandConsole(std::string_view cmd, const std::vector<std::string>& args, CommandError err) {
		io::g_out << io::Color::Red;

		switch (err) {
		case CommandError::UNKNOWN_COMMAND: 
			io::g_out << "Unknown command: " << cmd;
			break;
		case CommandError::NOT_ENOUGH_ARGUMENTS:
			io::g_out << "Not enough arguments for command: " << cmd << ", got " << args.size() << " arguments";
			break;
		case CommandError::TOO_MANY_ARGUMENTS:
			io::g_out << "Too much arguments for command: " << cmd << ", got " << args.size() << " arguments";
			break;
		default: break;
		}

		io::g_out << "\nType h or help for the list of possible commands" << io::Color::White << std::endl;
	}

	// Doing a move
	void consoleGo() {
		g_limits.reset();

		SearchResult result = rootSearch(g_board);
		if (result.best.isNullMove()) {
			return;
		}

		g_board.makeMove(result.best);
		g_limits.addMoves(1);
		g_moveHistory.push_back(result.best);

		io::g_out << "Best move: " << io::Color::Blue << result.best << io::Color::White
			<< "\nValue: " << io::Color::Green << result.value << io::Color::White << " cantipawns\n"
			<< g_board << std::endl;
	}

	void printHelp() {
		io::g_out << io::Color::Green
			<< "List of available commands: "\
			"\n\thelp/h - the information on commands available"\
			"\n\tquit/q - to quit the program"\
			"\n\tnew - to reset the board"\
			"\n\tsetfen [fen: FEN] - to reset the board and begin a game from the given position"\
			"\n\tfen - to print the FEN of the current position"\
			"\n\tboard/print - to show the current board"\
			"\n\tmoves - to get the list of possible moves"\
			"\n\tdo [move] - to make a move"\
			"\n\tundo - to unmake a move"\
			"\n\trandom - toggles the random mode, where the engine makes more random moves"\
			"\n\tforce - sets the force mode, where the engine doesn't make moves and only accepts input"\
			"\n\tlevel [control: uint] [base time: minutes:seconds] [inc time: seconds] - sets time limits"\
			"\n\tset_max_nodes [nodes: u64] - sets nodes limit"\
			"\n\tset_max_depth [depth: u64] - sets depth limit"\
			"\n\tgo - resets the force mode and starts the engine's move"\
			"\n\thistory - to print the moves done during the game"\
			"\n\teval - returns static evaluation of the current position"\
			"\n\tsearch [depth: uint] - returns the position evaluation based on search for given depth"\
			"\n\tperft [depth: uint] - starts the performance test for the given depth and prints the number of nodes"\
			"\n\t? - stops the current search and prints the results or makes a move immediately"
			<< io::Color::White << std::endl;
	}

	void trySetNewFen() {
		std::string currentFen = g_board.toFEN();
		auto currentMoveHistory = g_moveHistory;
		if (!newGame(io::getAllArguments())) {
			io::g_out << io::Color::Red << "Illegal position; the board was not changed" << io::Color::White << std::endl;

			bool _;
			g_board = Board::fromFEN(currentFen, _);
			g_moveHistory = currentMoveHistory;
		} else {
			io::g_out << io::Color::Green << "Position set successfully!" << io::Color::White << std::endl;
		}
	}

	bool handleConsole(std::string cmd, const std::vector<std::string>& args) {
		setIncorrectCommandCallback(handleIncorrectCommandConsole);
		SWITCH_CMD {
			CASE_CMD_WITH_VARIANT("help", "h", 0, 0) printHelp(); break;
			CASE_CMD_WITH_VARIANT("quit", "q", 0, 0) return false;
			CASE_CMD("new", 0, 0)
				options::g_isIllegalPosition = false;
				newGame(); 
				break;
			CASE_CMD("setfen", 1, 99) trySetNewFen(); break;
			CASE_CMD("fen", 0, 0)
				io::g_out << "Current position's FEN: " << io::Color::Blue << g_board.toFEN() << io::Color::White << std::endl;
				break;
			CASE_CMD_WITH_VARIANT("board", "print", 0, 0)
				io::g_out << "Current position:\n" << g_board << std::endl;
				break;
			CASE_CMD("moves", 0, 0) {
				io::g_out << "Available moves:\n" << io::Color::Green;
				MoveList moves;
				g_board.generateMoves(moves);

				for (auto m : moves) {
					if (g_board.isLegal(m)) {
						io::g_out << '\t' << m << std::endl;
					}
				}

				io::g_out << io::Color::White;
			} break;
			CASE_CMD("do", 1, 1)
				if (!makeMove(args[0])) {
					io::g_out << io::Color::Red << "Illegal move!" << io::Color::White << std::endl;
				} else if (!options::g_forceMode && !options::g_analyzeMode) {
					consoleGo();
				} break;
			CASE_CMD("undo", 0, 0)
				if (!unmakeMove()) {
					io::g_out << io::Color::Red << "Cannot unmake move: " << g_errorMessage << io::Color::White << std::endl;
				} break;
			CASE_CMD("random", 0, 0) options::g_randomMode = !options::g_randomMode; break;
			CASE_CMD("force", 0, 0) options::g_forceMode = true; break;
			CASE_CMD("level", 3, 3) {
				const u32 control = str_utils::fromString<u32>(args[0]);
				const u32 incTime = str_utils::fromString<u32>(args[2]);

				u32 i = 0;
				u32 baseTime = str_utils::fromString<u32>(args[1], i) * 60;
				if (i < args[1].size() && args[1][i] == ':') {
					baseTime += str_utils::fromString<u32>(args[1], ++i);
				}

				g_limits.setTimeLimits(control, baseTime, incTime);
			} break;
			CASE_CMD("set_max_nodes", 1, 1) g_limits.setNodesLimit(str_utils::fromString<u64>(args[0])); break;
			CASE_CMD("set_max_depth", 1, 1) g_limits.setDepthLimit(str_utils::fromString<u8>(args[0])); break;
			CASE_CMD("go", 0, 0) options::g_forceMode = false; consoleGo(); break;
			CASE_CMD("history", 0, 0)
				io::g_out << "History of the moves in the current game:\n" << io::Color::Green;
				for (auto m : g_moveHistory) {
					io::g_out << '\t' << m << std::endl;
				}

				io::g_out << io::Color::White;
				break;
			CASE_CMD("eval", 0, 0)
				io::g_out << "Evaluation: " << io::Color::Green << eval(g_board) << " cantipawns" << io::Color::White << std::endl;
				break;
			CASE_CMD("search", 1, 1) {
				Value result = search(g_board, -INF, INF, str_utils::fromString<u8>(args[0]), 0);
				io::g_out << "Search result: " << io::Color::Green << result << " cantipawns" << io::Color::White << std::endl;
			} break;
			CASE_CMD("perft", 1, 1) {
				auto nodes = engine::perft(g_board, str_utils::fromString<u8>(args[0]));
				io::g_out << "Nodes found: " << nodes << std::endl;
			} break;
			IGNORE_CMD("?")
			CMD_DEFAULT
		}

		return true;
	}

	void checkConsole(std::string cmd, const std::vector<std::string>& args) {
		const static Hash s_acceptedCommands[] = { // Commands that are handled right away
			HASH_OF("do"), HASH_OF("undo"), HASH_OF("?"), HASH_OF("q"), HASH_OF("quit")
		};

		if (!isOneOf(cmd, s_acceptedCommands)) {
			io::pushCommand(std::move(cmd), args);
			return;
		}

		if (cmd == "q" || cmd == "quit") {
			exit(0);
		} else { // do, undo, ?
			engine::stopSearching();
			if (cmd != "?") {
				io::pushCommand(std::move(cmd), args); // Would do/undo move in the main handling loop
			}
		}
	}
}