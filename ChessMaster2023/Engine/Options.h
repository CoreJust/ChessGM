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
#include "Utils/Types.h"

/*
*	Options(.h/.cpp) contains all the global options of the engine.
*/

namespace options {
	// Random mode adds a small value to the moves evaluation, thus increasing the
	// move choice spreading.
	extern bool g_randomMode;

	// Force mode prevents the engine from doing moves of its own.
	// In this mode, the engine just accepts the commands/makes the moves passed to it.
	extern bool g_forceMode;

	// Analyze mode is just what it seems to be.
	// The engine outputs all the thinking, but doesn't make any moves.
	extern bool g_analyzeMode;

	// Post mode enables printing of the current search state while thinking/pondering
	extern bool g_postMode;

	// Debug mode directs the engine's input and output, as well as some internal data,
	// into the log file.
	extern bool g_debugMode;


	///  OWN ENGINE STATE VARIABLES  ///

	// It is set to true when the engine is thinking, and set to false otherwise
	extern bool g_isThinking;

	// It is set when the user tries to set an illegal position.
	// With this, some of the commands would be denied unless a correct position is set.
	extern bool g_isIllegalPosition;

	// It is set when it is known that the engine's opponent is another version of the engine
	extern bool g_isPlayingAgainstSelf;

	// It is set when the engine is playing against another engine
	// In such a case, the engine would resign on sure-to-lose positions
	extern bool g_isComputerOpponent;
}