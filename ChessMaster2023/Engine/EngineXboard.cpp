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
#include "ChessMasterInfo.h"
#include "Search.h"
#include "Eval.h"

namespace engine {
	time_t g_timeLeft = 0;
	Value g_initialPositionValue = 0; // The evaluated value of the position the engine began the game from

	///  SOME OF XBOARD COMMANDS HANDLING  ///

	// If the game has ended, print the corresponding message to the GUI and returns true
	bool xboardCheckForGameOver() {
		if (g_board.lowMaterialDraw()) {
			io::g_out << "1/2-1/2 {Insufficient material}" << std::endl;
			return true;
		} else if (g_board.fiftyRuleDraw()) {
			io::g_out << "1/2-1/2 {50 move rule}" << std::endl;
			return true;
		} else if (g_board.repetitionDraw()) {
			io::g_out << "1/2-1/2 {Draw by repetition}" << std::endl;
			return true;
		}

		GameResult gr = g_board.computeGameResult();
		if (gr == GameResult::NONE) {
			return false;
		}

		if (gr == GameResult::WHITE_WON) {
			io::g_out << "1-0 {White mates}" << std::endl;
		} else if (gr == GameResult::BLACK_WON) {
			io::g_out << "0-1 {Black mates}" << std::endl;
		} else {
			io::g_out << "1/2-1/2 {Stalemate}" << std::endl;
		}

		return true;
	}

	// Makes engine's move in xboard mode
	void xboardGo() {
		g_limits.reset(g_timeLeft);

		SearchResult result = rootSearch(g_board);
		if (result.best.isNullMove()) {
			if (xboardCheckForGameOver()) {
				return;
			}
		}

		if (options::g_isComputerOpponent || options::g_isPlayingAgainstSelf) {
			// The engine is playing against another engine and it is evaluated
			// that we lost a value equal to ten pawns
			// Can resign here as well
			if (result.value + g_initialPositionValue < -scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE] * 10) {
				io::g_out << "resign" << std::endl;
				return;
			}
		}

		io::g_out << "move " << result.best << std::endl;
		g_board.makeMove(result.best);
		g_limits.addMoves(1);
		g_moveHistory.push_back(result.best);
	}

	void xboardAnalyze() {
		g_limits = Limits();
		options::g_postMode = true;

		while (true) {
			rootSearch(g_board);

			if (options::g_analyzeMode) {
				if (io::hasCommandsInQueue()) {
					std::vector<std::string> args;
					std::string cmd = io::getCommand(args, true);
					handleXboard(cmd, args);
				}
			} else {
				break;
			}
		}
	}

	///  ERROR HANDLING  ///

	// General function that is called by SWITCH_CMD
	void handleIncorrectCommandXboard(std::string_view cmd, const std::vector<std::string>& args, CommandError err) {
		switch (err) {
		case CommandError::UNKNOWN_COMMAND:
			io::g_out << "Error (unknown command): " << cmd << std::endl;
			break;
		case CommandError::NOT_ENOUGH_ARGUMENTS:
			io::g_out << "Error (not enough arguments): " << cmd << std::endl;
			break;
		case CommandError::TOO_MANY_ARGUMENTS:
			io::g_out << "Error (too many arguments): " << cmd << std::endl;
			break;
		default: break;
		}
	}

	void handleIllegalMove(std::string_view move) {
		io::g_out << "Illegal move: " << move << std::endl;
	}

	// Handles a command in xboard mode
	bool handleXboard(std::string cmd, const std::vector<std::string>& args) {
		setIncorrectCommandCallback(handleIncorrectCommandXboard);
		SWITCH_CMD {
			CASE_CMD_WITH_VARIANT("quit", "q", 0, 0) return false;
			CASE_CMD("rejected", 0, 2) return false;
			IGNORE_CMD("accepted") // The features were accepted
			CASE_CMD("new", 0, 0)
				options::g_isIllegalPosition = false;
				options::g_randomMode = false;
				options::g_forceMode = false;

				g_limits.makeInfinite(); // Reseting all the limits
				g_initialPositionValue = 0;
				newGame();
				break;
			CASE_CMD("random", 0, 0) options::g_randomMode = !options::g_randomMode; break;
			CASE_CMD("force", 0, 0) options::g_forceMode = true; break;
			CASE_CMD("go", 0, 0) options::g_forceMode = false; xboardGo(); break;
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
			CASE_CMD("st", 1, 1) g_limits.setTimeLimits(0, 0, str_utils::fromString<u32>(args[0])); break;
			CASE_CMD("sd", 1, 1) g_limits.setDepthLimit(str_utils::fromString<u8>(args[0])); break;
			CASE_CMD("nps", 1, 1) break; // TODO: implement
			CASE_CMD("time", 1, 1) g_timeLeft = str_utils::fromString<u32>(args[0]) * 10; break;
			IGNORE_CMD("otim")
			CASE_CMD("usermove", 1, 1) 
				if (options::g_isIllegalPosition || !makeMove(args[0])) {
					handleIllegalMove(args[0]); 
				} else if (!xboardCheckForGameOver() && !options::g_forceMode && !options::g_analyzeMode) {
					xboardGo();
				} break;
			CASE_CMD("?", 0, 0) break; // Ignored if got not during searching
			CASE_CMD("ping", 1, 1) 
				if (!options::g_isThinking) {
					io::g_out << "pong " << args[0] << std::endl;
				} break;
			CASE_CMD("draw", 0, 0) io::g_out << "offer draw" << std::endl; break;
			IGNORE_CMD("result") // Should declare the result of the game
			CASE_CMD("setboard", 1, 99) 
				if (!newGame(io::getAllArguments())) {
					io::g_out << "tellusererror Illegal position" << std::endl;
					options::g_isIllegalPosition = true;
				} else {
					options::g_isIllegalPosition = false;
					options::g_randomMode = false;

					g_initialPositionValue = eval(g_board);
				} break;
			CASE_CMD("hint", 0, 0) break; // TODO: implement hinting
			IGNORE_CMD("bk") // When the user chooses the "Book" option in the menu
			CASE_CMD("undo", 0, 0)
				if (!unmakeMove()) {
					io::g_out << "Error (undo is illegal now): " << g_errorMessage << std::endl;
				} break;
			CASE_CMD("remove", 0, 0)
				if (!unmakeMove() || !unmakeMove()) {
					io::g_out << "Error (remove is illegal now): " << g_errorMessage << std::endl;
				} break;
			IGNORE_CMD("hard") // Should turn pondering on
			IGNORE_CMD("easy") // Should turn pondering off
			CASE_CMD("post", 0, 0) options::g_postMode = true; break;
			CASE_CMD("nopost", 0, 0) options::g_postMode = true; break;
			CASE_CMD("analyze", 0, 0)
				if (!options::g_analyzeMode) {
					options::g_analyzeMode = true;
					xboardAnalyze();
				} break;
			IGNORE_CMD("exit")
			CASE_CMD("name", 1, 999) {
				options::g_isPlayingAgainstSelf = (io::getAllArguments().find(ENGINE_NAME) != std::string_view::npos);
			} break;
			IGNORE_CMD("rating") // Should inform about opponent's and engine's rating
			IGNORE_CMD("ics") // Should inform about whether the opponent is local or online
			CASE_CMD("computer", 0, 0) options::g_isComputerOpponent = true; break;
			CMD_DEFAULT
		}

		return true;
	}

	void checkXboard(std::string cmd, const std::vector<std::string>& args) {
		const static Hash s_acceptedCommands[] = { // Commands that are handled right away
			HASH_OF("usermove"), HASH_OF("undo"), HASH_OF("new"), HASH_OF("setboard"), HASH_OF("exit"), 
			HASH_OF("."), HASH_OF("?"), HASH_OF("q"), HASH_OF("quit")
		};

		if (!isOneOf(cmd, s_acceptedCommands)) {
			io::pushCommand(std::move(cmd), args);
			return;
		}

		if (cmd == "exit") {
			options::g_analyzeMode = false;
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