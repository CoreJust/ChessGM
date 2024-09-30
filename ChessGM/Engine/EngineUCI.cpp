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

#include "Engine.h"
#include <algorithm>

#include "Utils/CommandHandlingUtils.h"
#include "Utils/StringUtils.h"
#include "Search.h"
#include "TranspositionTable.h"

namespace engine {
	void uciGo() {
		SearchResult result = rootSearch(g_board);

		io::g_out << "bestmove " << result.best << std::endl;
		g_board.makeMove(result.best);
		g_limits.addMoves(1);
		g_moveHistory.push_back(result.best);
	}

	void handleIncorrectCommandUCI(std::string_view cmd, const std::vector<std::string>& args, CommandError err) {
		// Nothing here
	}

	bool handleUCI(std::string cmd, const std::vector<std::string>& args) {
		setIncorrectCommandCallback(handleIncorrectCommandUCI);
		SWITCH_CMD {
			CASE_CMD_WITH_VARIANT("quit", "q", 0, 0) return false;
			CASE_CMD("debug", 1, 1) options::g_debugMode = (args[0] == "on"); break;
			CASE_CMD("isready", 0, 0) io::g_out << "readyok" << std::endl; break;
			CASE_CMD("setoption", 4, 6) {
				if (args[0] == "name" && args[1] == "Hash" && args[3] == "value") {
					engine::TranspositionTable::setSize(atoi(args[4].c_str()));
				}
			} break;
			IGNORE_CMD("register")
			CASE_CMD("ucinewgame", 0, 0) {
				// May do something to reset the search state
			} break;
			CASE_CMD("position", 1, 9999) {
				g_moveHistory.clear();
				if (args[0] == "startpos") {
					newGame();
				} else { // args[0] = "fen"
					std::string_view str = io::getAllArguments().substr();
					std::string_view fen = str.substr(4, str.find("moves") - 6);
					newGame(fen);
				}

				if (auto movesFrom = std::find(args.begin(), args.end(), "moves"); movesFrom != args.end()) {
					while (++movesFrom != args.end()) {
						makeMove(*movesFrom);
					}
				}
			} break;
			CASE_CMD("go", 0, 9999) {
				u32 movesTillControl = 0;
				time_t incTime = 0;
				time_t timeLeft = 0;

				for (auto it = args.begin(); it != args.end(); it++) {
					if (*it == "infinite") {
						g_limits = Limits();
					} else if (*it == "movetime") {
						time_t msForMove = str_utils::fromString<u64>(*(++it));
						g_limits.setTimeLimitsInMs(0, 0, msForMove);
						g_limits.reset(msForMove);
					} else if (*it == "nodes") {
						g_limits.setNodesLimit(str_utils::fromString<u64>(*(++it)));
					} else if (*it == "depth") {
						g_limits.setDepthLimit(static_cast<Depth>(str_utils::fromString<u32>(*(++it))));
					} else if (*it == "movestogo") {
						movesTillControl = str_utils::fromString<u32>(*(++it));
					} else if ((*it == "winc" && g_board.side() == Color::WHITE)
							   || (*it == "binc" && g_board.side() == Color::BLACK)) {
						incTime = str_utils::fromString<u32>(*(++it));
					} else if ((*it == "wtime" && g_board.side() == Color::WHITE)
							 || (*it == "btime" && g_board.side() == Color::BLACK)) {
						timeLeft = str_utils::fromString<u32>(*(++it));
					} // TODO: implement searchmoves, ponder, mate
				}

				if (movesTillControl || incTime) {
					g_limits.setTimeLimitsInMs(movesTillControl, timeLeft, incTime);
				}

				if (timeLeft) {
					g_limits.reset(timeLeft);
				}

				uciGo();
			} break;
			IGNORE_CMD("stop")
			IGNORE_CMD("ponderhit")
			CMD_DEFAULT
		}

		return true;
	}

	void checkUCI(std::string cmd, const std::vector<std::string>& args) {
		const static Hash s_acceptedCommands[] = { // Commands that are handled right away
			HASH_OF("stop"), HASH_OF("quit")
		};

		if (!isOneOf(cmd, s_acceptedCommands)) {
			io::pushCommand(std::move(cmd), args);
			return;
		}

		if (cmd == "quit") {
			exit(0);
		} else { // stop
			engine::stopSearching();
		}
	}
}