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
#include <cctype>
#include <string_view>
#include <vector>
#include <numeric>
#include <concepts>

#include "Types.h"

#undef max

/*
*	StringUtils.h contains a set of utils to work with strings.
*/

namespace str_utils {
	CM_PURE constexpr bool isDigit(const char ch) noexcept {
		return ch >= '0' && ch <= '9';
	}

	// Tries to convert the number from <str>.
	// Returns 0 if the first character is not a digit or str is empty.
	// Converts only as long as the value fits into the requested type.
	template<std::integral T> requires std::is_unsigned_v<T>
	CM_PURE constexpr T fromString(std::string_view str, u32& i) noexcept {
		// The maximal value that can accept another digit
		constexpr T PREVIOUS_VALUE_MAX = std::numeric_limits<T>::max() / 10;

		if (str.empty()) {
			return T(0);
		}

		T result = 0;
		for (; i < str.size(); ++i) {
			if (!isDigit(str[i])) {
				return result;
			}

			result *= 10;
			result += T(str[i] - '0');

			if (result >= PREVIOUS_VALUE_MAX) {
				return result;
			}
		}

		return result;
	}

	template<std::integral T> requires std::is_unsigned_v<T>
	CM_PURE constexpr T fromString(std::string_view str) noexcept {
		u32 _ = 0;
		return fromString<T>(str, _);
	}

	std::vector<std::string_view> split(std::string_view str, std::string_view delimiters, std::vector<std::string_view> vec = {});
}