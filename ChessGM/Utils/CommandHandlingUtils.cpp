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

#include "CommandHandlingUtils.h"

void defaultHIC(std::string_view cmd, const std::vector<std::string>& args, CommandError err) {
	
}

IncorrectCommandHandleFunc g_HICfunc = defaultHIC;

void setIncorrectCommandCallback(IncorrectCommandHandleFunc func) {
	g_HICfunc = func;
}

IncorrectCommandHandleFunc _getHIC() {
	return g_HICfunc;
}
