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
#include "Defs.h"

/*
*	Move.h contains a class that represents a single chess move.
*	
*	The move info is packed in m_data as follows:
*		bits 0-5:   from
*		bits 6-11:  to
*		bits 12-13: promoted piece
*		bits 14-15: move type
* 
*	Additionally, there is an auxiliary type MoveType that represents the kinf of a move.
*/


WRAPPED_ENUM_BEGIN(MoveType, u8)
	SIMPLE = 0,
	PROMOTION,
	ENPASSANT,
	CASTLE,
WRAPPED_ENUM_IMPL
WRAPPED_ENUM_END(MoveType, u8)



class Move final {
private:
	union {
		u32 m_asU32;
		struct {
			u16 m_data;
			Value m_value;
		};
	};

public:
	INLINE constexpr Move() noexcept : m_asU32(0) { }
	INLINE constexpr Move(const Move& other) noexcept : m_asU32(other.m_asU32) { }
	INLINE constexpr explicit Move(const u32 value) noexcept : m_asU32(value) { }
	
	INLINE constexpr Move(const Square from, const Square to) noexcept
		: Move(u32(from) | (u32(to) << 6)) { }

	INLINE constexpr Move(const Square from, const Square to, const MoveType mt, const PieceType promotedPiece = PieceType::KNIGHT) noexcept
		: Move(u32(from) | (u32(to) << 6) | (u32(promotedPiece - PieceType::KNIGHT) << 12) | (u32(mt) << 14)) { }

	INLINE constexpr Move& operator=(const Move& other) noexcept {
		m_asU32 = other.m_asU32;
		return *this;
	}

	CM_PURE constexpr bool operator==(const Move other) const noexcept {
		return m_data == other.m_data;
	}

	CM_PURE consteval static Move makeNullMove() noexcept {
		return Move();
	}

	CM_PURE constexpr static Move fromData(const u16 data) noexcept {
		Move result = makeNullMove();
		result.m_data = data;

		return result;
	}

	CM_PURE constexpr bool isNullMove() const noexcept {
		return m_data == 0;
	}

	CM_PURE constexpr u16 getData() const noexcept {
		return m_data;
	}

	INLINE constexpr void setValue(const Value value) noexcept {
		m_value = value;
	}

	CM_PURE constexpr Value getValue() const noexcept {
		return m_value;
	}

	CM_PURE constexpr u32 asU32() const noexcept {
		return m_asU32;
	}
	
	CM_PURE constexpr Square getFrom() const noexcept {
		return Square::Value(m_data & 0x3f);
	}

	CM_PURE constexpr Square getTo() const noexcept {
		return Square::Value((m_data >> 6) & 0x3f);
	}

	CM_PURE constexpr PieceType getPromotedPiece() const noexcept {
		return PieceType::Value(((m_data >> 12) & 0x3) + PieceType::KNIGHT);
	}

	CM_PURE constexpr MoveType getMoveType() const noexcept {
		return MoveType::Value((m_data >> 14) & 0x3);
	}

	CM_PURE std::string toString() const noexcept {
		switch (getMoveType()) {
			case MoveType::SIMPLE:
			case MoveType::CASTLE:
			case MoveType::ENPASSANT: 
				return std::string(getFrom().toString())
					.append(getTo().toString());
			case MoveType::PROMOTION: 
				return std::string(getFrom().toString())
					.append(getTo().toString())
					.append(1, getPromotedPiece().toChar());
		default: return "";
		}
	}
};

static_assert(sizeof(Move) == sizeof(u32));
