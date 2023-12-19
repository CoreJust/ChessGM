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
#include "Chess/Defs.h"
#include "Scores.h"

/*
*	TranspositionTable(.h/.cpp) contains the implementation of, well, transposition table.
* 
*	A transposition table is a hash table used to store search hash:
*		the score, the best move, some data to correctly use those two.
*/

namespace engine {
	enum EntryType : u8 {
		NON_PV = 0,
		PV = 1,

		EXACT = 0b010,
		BETA = 0b100,
		ALPHA = 0b110
	};

	// A single record in the transposition table
	struct TableEntry final {
		Hash hash;	 //	8b | Used to ensure that the found position in the table is what we looked for
		u16 move;	 //	2b | The best move (only the move data without its score)
		Value value; // 2b | The found position value
		u16 age;	 // 2b | The age of the entry is used to replace the old nodes that cannot be used anymore. Actually, it is the move count of the position
		u8 depth;	 // 1b | The depth where the entry was recorded
		EntryType type; // 1b | The type of the entry, allows to understand when to use/replace the node

		CM_PURE constexpr bool isPvNode() const noexcept {
			return type & PV;
		}

		CM_PURE constexpr EntryType getBoundType() const noexcept {
			return EntryType(type & 0b110);
		}
	};

	static_assert(sizeof(TableEntry) == 16);

	// ChessMaster uses several entries for the same hash with different replacement policies.
	struct TableEntryCluster final {
		// Main entry is replaced only with entries with bigger depth
		TableEntry mainEntry;

		// Auxiliary entry is replaced always
		TableEntry auxEntry;
	};

	static_assert(sizeof(TableEntryCluster) == sizeof(TableEntry) * 2);

	// The class of the transposition table. Contains an array of TableEntries and manages it.
	class TranspositionTable final {
	public:
		// Default table size in bytes.
		constexpr inline static uint32_t DEFAULT_TABLE_SIZE = 64 * 1024 * 1024;

	private:
		static TableEntryCluster* s_table;
		static uint32_t s_tableSize;
		static u16 s_rootAge;

	public:
		static void init();
		static void destroy();

		INLINE static void setRootAge(const u16 age) {
			s_rootAge = age;
		}

		// Looks for the record in the table
		// Returns an entry if it was found and nullptr otherwise
		CM_PURE static TableEntry* probe(const Hash hash) noexcept {
			assert(s_tableSize != 0);

			TableEntryCluster* entry = &s_table[hash % s_tableSize];
			return entry->mainEntry.hash == hash
					? &entry->mainEntry
				: entry->auxEntry.hash == hash
					? &entry->auxEntry
					: nullptr;
		}

		// Naive Always-Replace strategy
		INLINE static void tryRecord(
			const EntryType type, 
			const Hash hash,
			const u16 move,
			Value value, 
			const u16 age, 
			const u8 depth, 
			const Depth ply
		) {
			assert(s_tableSize != 0);

			TableEntryCluster* entry = &s_table[hash % s_tableSize];
			TableEntry& mainEntry = entry->mainEntry;

			// Checking if the old main entry must be replaced
			if (
				mainEntry.type == 0 // If type is 0 then it is a never-used empty entry
				|| mainEntry.age <= s_rootAge // The entry already aged
				|| depth > mainEntry.depth // A more deep searched entry is prefered
				|| (depth == mainEntry.depth						// If the entry is as deep as an already found
					&& (type & PV) >= (mainEntry.type & PV)		// and it is not a Non-PV node while the written one is PV
					&& (type & 0x110) <= mainEntry.getBoundType()) // and its bound type is better, than we overwrite the entry
			) {
				if (isMateValue(value)) { // Fixing the mate values
					if (value > MATE - 2 * MAX_DEPTH) {
						value += ply;
					} else {
						value -= ply;
					}
				}

				new(&mainEntry) TableEntry { .hash = hash, .move = move, .value = value, .age = age, .depth = depth, .type = type };
			} else if (mainEntry.hash != hash) { // Otherwise, check if the auxiliary entry must be replaced
				new(&entry->auxEntry) TableEntry { .hash = hash, .move = move, .value = value, .age = age, .depth = depth, .type = type };
			}
		}
	};
}