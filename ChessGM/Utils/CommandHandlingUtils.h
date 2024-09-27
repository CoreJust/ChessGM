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
#include <string_view>
#include <string>
#include <vector>

#include "Types.h"

/*
*	CommandHandlingUtils.h provides some utilities for processing input
*	commands.
* 
*	The general utils is SWITCH_CMD.
* 
*	Usage:
*		SWITCH_CMD {
*			CASE_CMD(<some compile-time known command>, <minimal number of command args>, <maximal number of command args>) {
*				// Some handling
*			} break;
*			...
*			CMD_DEFAULT
*		}
* 
*	Note: 
*		Arguments must be in a vector of string_view's with name "args", 
*		and the command must be a string_view in a variable called "cmd"
*/

enum class CommandError : u8 {
	UNKNOWN_COMMAND = 0,
	NOT_ENOUGH_ARGUMENTS,
	TOO_MANY_ARGUMENTS
};

using IncorrectCommandHandleFunc = void (*)(std::string_view, const std::vector<std::string>&, CommandError);

void setIncorrectCommandCallback(IncorrectCommandHandleFunc func);
IncorrectCommandHandleFunc _getHIC();

// Computes the hash of a string in runtime
// No collisions for string with size of up to 9 characters
CM_PURE constexpr Hash _computeDynamicHash(std::string_view str) noexcept {
	Hash result = 0;
	u8 offset = 0;

	for (char ch : str) {
		result ^= Hash(ch) << offset;
		if ((offset += 7) > 57) {
			offset = 0;
		}
	}

	return result;
}

// Same as _computeDynamicHash, but for compile-time only
CM_PURE consteval Hash _computeStaticHash(std::string_view str) noexcept {
	Hash result = 0;
	u8 offset = 0;

	for (char ch : str) {
		result ^= Hash(ch) << offset;
		if ((offset += 7) > 57) {
			offset = 0;
		}
	}

	return result;
}


#define SWITCH_CMD switch(_computeDynamicHash(cmd))
#define CASE_CMD(_cmd, minArgs, maxArgs) case _computeStaticHash(_cmd): \
	if(args.size() < (minArgs)) {\
		_getHIC()(cmd, args, CommandError::NOT_ENOUGH_ARGUMENTS);\
		break;\
	} else if (args.size() > (maxArgs)) {\
		_getHIC()(cmd, args, CommandError::TOO_MANY_ARGUMENTS);\
		break;\
	}

// Allows 2 commands: the full-length one and a short one (although not necessarily so, they can be any)
#define CASE_CMD_WITH_VARIANT(_cmd, _shortCmd, minArgs, maxArgs) case _computeStaticHash(_cmd): case _computeStaticHash(_shortCmd):\
	if(args.size() < (minArgs)) {\
		_getHIC()(cmd, args, CommandError::NOT_ENOUGH_ARGUMENTS);\
		break;\
	} else if (args.size() > (maxArgs)) {\
		_getHIC()(cmd, args, CommandError::TOO_MANY_ARGUMENTS);\
		break;\
	}

#define IGNORE_CMD(_cmd) case _computeStaticHash(_cmd): break;

#define CMD_DEFAULT default: _getHIC()(cmd, args, CommandError::UNKNOWN_COMMAND); break;


// Utils for checking if the string is one of an array

#define HASH_OF(_cmd) _computeStaticHash(_cmd)

template<size_t Size>
constexpr bool _containsHash(const Hash hash, const Hash(&hashes)[Size]) noexcept {
	for (Hash h : hashes) {
		if (h == hash) {
			return true;
		}
	}

	return false;
}

template<size_t Size>
constexpr bool isOneOf(std::string_view str, const Hash(&hashes)[Size]) noexcept {
	return _containsHash(_computeDynamicHash(str), hashes);
}