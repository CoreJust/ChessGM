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

#include "TranspositionTable.h"
#include <cstring>

namespace engine {
	TableEntryCluster* TranspositionTable::s_table = nullptr;
	uint32_t TranspositionTable::s_tableSize = 0;
	u16 TranspositionTable::s_rootAge = 0;

	void TranspositionTable::init() {
		s_table = reinterpret_cast<TableEntryCluster*>(malloc(DEFAULT_TABLE_SIZE));
		assert(s_table != nullptr);

		memset(s_table, 0, DEFAULT_TABLE_SIZE);
		s_tableSize = DEFAULT_TABLE_SIZE / sizeof(TableEntryCluster);
	}

	void TranspositionTable::destroy() { 
		if (s_table) {
			free(s_table);
			s_table = nullptr;
			s_tableSize = 0;
		}
	}
}
