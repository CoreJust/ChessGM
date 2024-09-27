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

#include "StringUtils.h"

namespace str_utils {
	std::vector<std::string_view> split(std::string_view str, std::string_view delimiters, std::vector<std::string_view> vec) {
		using Iterator = decltype(str.begin());

		vec.clear();

		Iterator it = str.begin();
		while (it != str.end()) {
			// Skipping delimiters
			while (delimiters.find(*it) != std::string_view::npos && ++it != str.end());

			// Reading another element
			if (it != str.end()) {
				Iterator first = it;
				while (delimiters.find(*it) == std::string_view::npos && ++it != str.end());

				vec.emplace_back(first, it);
			}
		}

		return vec;
	}
}
