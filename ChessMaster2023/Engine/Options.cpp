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

#include "Options.h"

namespace options {
	bool g_randomMode = false; 
	bool g_forceMode = false;
	bool g_analyzeMode = false;
	bool g_postMode = true;
	bool g_debugMode = false;

	bool g_isThinking = false;
	bool g_isIllegalPosition = false;
	bool g_isPlayingAgainstSelf = false;
}