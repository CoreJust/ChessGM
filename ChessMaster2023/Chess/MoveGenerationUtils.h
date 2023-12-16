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
#include "Move.h"

/*
*	MoveGenerationUtils.h contains some utils for move generation.
* 
*	The complete move generation function can be found within
*	the Board class.
* 
*	Contains the MoveList class and the movegen namespace.
*/

// An optimized list of moves from a single position.
// Contains all the moves in an array, so no memory
//		allocation/reallocation is required.
// Allows iteration over the list.
class MoveList final {
public:
	// The first fitting power of 2
	// The actual maximal number of moves possible from a legal position
	//		is 218 moves. The position is: R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - - 0 1
	constexpr inline static u32 MAX_MOVES = 256;

private:
	Move m_data[MAX_MOVES];
	Move* m_end;

public:
	INLINE constexpr MoveList() noexcept {
		for (u32 i = 0; i < MAX_MOVES; i++) {
			m_data[i] = Move();
		}

		m_end = m_data;
	}

	MoveList(const MoveList&) = delete;
	MoveList(MoveList&&) = delete;

	CM_PURE constexpr Move& operator[](const u32 index) noexcept {
		assert(m_data + index < m_end);
		
		return m_data[index];
	}

	CM_PURE constexpr Move operator[](const u32 index) const noexcept {
		assert(m_data + index < m_end);

		return m_data[index];
	}

	// Inserts the contents of the given list into this one starting from <from>
	INLINE constexpr void mergeWith(const MoveList& other, const u32 from) noexcept {
		assert(this != &other);

		const u32 to = std::min(MAX_MOVES, from + other.size());
		if (std::is_constant_evaluated()) {
			for (u32 i = from, j = 0; i < to; ++i, ++j) {
				m_data[i] = other[j];
			}
		} else {
			memcpy(m_data + from, other.m_data, (to - from) * sizeof(Move));
		}

		m_end = m_data + to;
	}

	INLINE constexpr void push(const Move move) noexcept {
		assert(m_end < m_data + MAX_MOVES);

		*(m_end++) = move;
	}

	INLINE void emplace(const Square from, const Square to) noexcept {
		assert(m_end < m_data + MAX_MOVES);

		::new(m_end++) Move(from, to);
	}

	template<MoveType::Value MT>
	INLINE void emplace(const Square from, const Square to, const PieceType promotedPiece = PieceType::KNIGHT) noexcept {
		assert(m_end < m_data + MAX_MOVES);

		::new(m_end++) Move(from, to, MT, promotedPiece);
	}

	INLINE constexpr void clear() noexcept {
		m_end = m_data;
	}

	CM_PURE constexpr Move* begin() noexcept {
		return m_data;
	}

	CM_PURE constexpr const Move* begin() const noexcept {
		return m_data;
	}

	CM_PURE constexpr Move* end() noexcept {
		return m_end;
	}

	CM_PURE constexpr const Move* end() const noexcept {
		return m_end;
	}

	CM_PURE constexpr u32 size() const noexcept {
		return m_end - m_data;
	}

	// Creates a line of the moves, like: e2e4 c7c5 g1f3...
	// If the first move is given, the line begins with it
	CM_PURE std::string toString(const Move first = Move::makeNullMove()) const noexcept {
		std::string result = first.isNullMove() ? std::string("") : (first.toString() + " ");
		result.reserve(result.size() + size() * 5);

		for (Move m : *this) {
			result += m.toString() + " ";
		}

		return result;
	}
};

// The functions to for move generation
// Uses templates for acceleration
namespace movegen {
	enum GenerationMode : ufast8 {
		ALL_MOVES, // Generating all the pseudo-legal moves
		CAPTURES, // Generating only captures and queen promotions
		CHECK_EVASIONS, // Generating moves while in check
		QUIET_CHECKS // Non-capturing checks (so as not to generate moves as in captures)
	};
}
