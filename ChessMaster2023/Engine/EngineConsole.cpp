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
#include <chrono>

#include "Utils/CommandHandlingUtils.h"
#include "Utils/StringUtils.h"
#include "Eval.h"
#include "Search.h"
#include "Test.h"
#include "Tuning.h"

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

		io::g_out << "\nType h or help for the list of possible commands" << std::endl;
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

		io::g_out << "Best move: " << io::Color::Blue << result.best << std::endl
			<< "Value: " << io::Color::Green << result.value << io::Color::White << " centipawns\n"
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
			"\n\tmoves [optional: all|captures|checks] - to get the list of possible moves"\
			"\n\tdo [move] - to make a move"\
			"\n\tundo - to unmake a move"\
			"\n\trandom - toggles the random mode, where the engine makes more random moves"\
			"\n\tforce - sets the force mode, where the engine doesn't make moves and only accepts input"\
			"\n\tlevel [control: uint] [base time: minutes:seconds] [inc time: seconds] - sets time limits"\
			"\n\tset_max_nodes [nodes: u64] - sets nodes limit"\
			"\n\tset_max_depth [depth: u64] - sets depth limit"\
			"\n\treset_limits - resets all the limits, making the search infinite"\
			"\n\tgo - resets the force mode and starts the engine's move"\
			"\n\thistory - to print the moves done during the game"\
			"\n\teval - returns static evaluation of the current position"\
			"\n\tsearch [depth: uint] - returns the position evaluation based on search for given depth"\
			"\n\tperft [depth: uint] - starts the performance test for the given depth and prints the number of nodes"\
			"\n\t? - stops the current search and prints the results or makes a move immediately"\
			"\n\ttest - developer's command, runs all the tests"\
			"\n\tcompute_eval_err/ceerr [optinal: filename, default: test_suit.fen] - conputes the error of static evaluation for the given positions"\
			"\n\textract_positions [from: pgn file] [to: fen file, test_suit.fen by default] - extracts positions suitable for ceerr"
			<< std::endl;
	}

	void trySetNewFen() {
		std::string currentFen = g_board.toFEN();
		auto currentMoveHistory = g_moveHistory;
		if (!newGame(io::getAllArguments())) {
			io::g_out << io::Color::Red << "Illegal position; the board was not changed" << std::endl;

			bool _;
			g_board = Board::fromFEN(currentFen, _);
			g_moveHistory = currentMoveHistory;
		} else {
			io::g_out << io::Color::Green << "Position set successfully!" << std::endl;
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
				io::g_out << "Current position's FEN: " << io::Color::Blue << g_board.toFEN() << std::endl;
				break;
			CASE_CMD_WITH_VARIANT("board", "print", 0, 0)
				io::g_out << "Current position:\n" << g_board << std::endl;
				break;
			CASE_CMD("moves", 0, 1) {
				MoveList moves;
				u32 legalMoves = 0;

				if (args.size() == 0 || args[0] == "all") {
					g_board.generateMoves(moves);
				} else if (args[0] == "captures") {
					g_board.generateMoves<movegen::CAPTURES>(moves);
				} else if (args[0] == "checks") {
					g_board.generateMoves<movegen::QUIET_CHECKS>(moves);
				}

				io::g_out << "Available moves:" << io::Color::Green;
				for (auto m : moves) {
					if (g_board.isLegal(m)) {
						legalMoves++;
						io::g_out << "\n\t" << m;
					}
				}

				io::g_out << std::endl << "Total moves: " << io::Color::Blue 
					<< legalMoves << std::endl;
			} break;
			CASE_CMD("do", 1, 1)
				if (!makeMove(args[0])) {
					io::g_out << io::Color::Red << "Illegal move!" << std::endl;
				} else if (!options::g_forceMode && !options::g_analyzeMode) {
					consoleGo();
				} break;
			CASE_CMD("undo", 0, 0)
				if (!unmakeMove()) {
					io::g_out << io::Color::Red << "Cannot unmake move: " << g_errorMessage << std::endl;
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
			CASE_CMD("reset_limits", 0, 0) g_limits.makeInfinite(); break;
			CASE_CMD("go", 0, 0) options::g_forceMode = false; consoleGo(); break;
			CASE_CMD("history", 0, 0)
				io::g_out << "History of the moves in the current game (" << g_moveHistory.size() << " moves made):" 
					<< io::Color::Green;
				for (auto m : g_moveHistory) {
					io::g_out << "\n\t" << m;
				}

				io::g_out << std::endl;
				break;
			CASE_CMD("eval", 0, 0)
				io::g_out << "Evaluation: " << io::Color::Green << eval(g_board) << " centipawns" << std::endl;
				break;
			CASE_CMD("search", 1, 1) {
				Value result = search(g_board, -INF, INF, str_utils::fromString<u8>(args[0]), 0);
				io::g_out << "Search result: " << io::Color::Green << result << " centipawns" << std::endl;
			} break;
			CASE_CMD("perft", 1, 1) {
				using namespace std::chrono;

				auto start = high_resolution_clock::now();
				NodesCount nodes = engine::perft(g_board, str_utils::fromString<u8>(args[0]));
				auto perftTime = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();

				double perftTimeInSeconds = perftTime / 1'000'000'000.0;
				double kiloNodesPerSecond = nodes / (perftTimeInSeconds * 1000);

				io::g_out << "Nodes found: " << io::Color::Blue << nodes << std::endl
					<< "Time: " << io::Color::Blue << perftTimeInSeconds << io::Color::White << " seconds" << std::endl
					<< "Kn/S: " << io::Color::Blue << kiloNodesPerSecond << io::Color::White << " kilonodes per second" << std::endl;
			} break;
			IGNORE_CMD("?")
			CASE_CMD("test", 0, 0) {
				runTests();
			} break;
			CASE_CMD_WITH_VARIANT("compute_eval_err", "ceerr", 0, 1) {
				std::string fileName = args.size() > 0 ? args[0] : "test_suit.fen";
				Tuning tuning;
				
				tuning.loadPositions(fileName);
				double err = tuning.computeErr();

				io::g_out << "Evaluation error: " << io::Color::Blue << std::setprecision(10) << err << std::endl;
			} break;
			CASE_CMD("extract_positions", 1, 2) {
				std::string pgnFileName = args[0];
				std::string fenFileName = args.size() > 1 ? args[1] : "test_suit.fen";

				Tuning::extractPositions(pgnFileName, fenFileName);
			} break;
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