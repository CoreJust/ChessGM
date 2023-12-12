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
#include <concepts>
#include "Defs.h"

/*
*	Score.h contains a class that represents a pair of score - 
*	one for the opening/middlegame and the other for endgame.
* 
*	Score is expected to be used for temporary values during evaluetion.
* 
*	Additionally, there is an auxiliary class Material that allows to account
*	the material for collapsing the Score. Generally, it just clamps the amount
*	of material.
*/

class Material final {
public:
	constexpr inline static i32 MIN = 8;
	constexpr inline static i32 RANGE = 64;
	constexpr inline static i32 MAX = MIN + RANGE;

private:
	i32 m_value;

public:
	INLINE constexpr Material(const i32 value) noexcept 
		: m_value((value > MAX ? MAX : value < MIN ? MIN : value) - MIN) { }

	// Interpolate [a, b] according to where the material in [MIN, MAX] is
	template<class T> requires(std::is_arithmetic_v<T>)
	CM_PURE constexpr T interpolate(const T a, const T b) const noexcept	{
		if constexpr (std::is_integral_v<T> && sizeof(T) < sizeof(i32)) {
			return T((i32(a) * m_value + i32(b) * (RANGE - m_value)) / RANGE);
		} else {
			return (a * m_value + b * (RANGE - m_value)) / RANGE;
		}
	}

public:
	CM_PURE constexpr static i32 materialOf(const PieceType piece) noexcept {
		const i32 MATERIAL_VALUES[PieceType::VALUES_COUNT] = {
			0, // None
			1, // Pawn
			3, // Knight
			3, // Bishop
			5, // Rook
			10, // Queen
			0 // King
		};

		return MATERIAL_VALUES[piece];
	}
};

class Score final {
private:
	union {
		u32 m_asU32;
		struct {
			Value m_middlegameValue;
			Value m_endgameValue;
		};
	};

public:
	INLINE constexpr Score() noexcept : m_asU32(0) { }
	INLINE constexpr Score(const Score& other) noexcept : m_asU32(other.m_asU32) { }
	INLINE constexpr explicit Score(const u32 asU32) noexcept : m_asU32(asU32) { }
	INLINE constexpr Score(const Value middlegame, const Value endgame) noexcept
		: m_middlegameValue(middlegame), m_endgameValue(endgame) { }

	INLINE constexpr Score& operator=(const Score other) noexcept {
		m_asU32 = other.m_asU32;
		return *this;
	}

	CM_PURE constexpr bool operator==(const Score other) const noexcept {
		return m_asU32 == other.m_asU32;
	}

	CM_PURE constexpr Score operator-() const noexcept {
		return Score(-m_middlegameValue, -m_endgameValue);
	}

	CM_PURE constexpr Score operator+(const Score other) const noexcept {
		return Score(m_asU32 + other.m_asU32);
	}

	CM_PURE constexpr Score operator-(const Score other) const noexcept {
		return Score(m_asU32 - other.m_asU32);
	}

	INLINE constexpr Score& operator+=(const Score other) noexcept {
		m_asU32 += other.m_asU32;
		return *this;
	}

	INLINE constexpr Score& operator-=(const Score other) noexcept {
		m_asU32 -= other.m_asU32;
		return *this;
	}

	CM_PURE constexpr Value middlegame() const noexcept {
		return m_middlegameValue;
	}

	CM_PURE constexpr Value endgame() const noexcept {
		return m_endgameValue;
	}

	CM_PURE constexpr Value collapse(const Material material) const noexcept {
		return material.interpolate(m_middlegameValue, m_endgameValue);
	}
};

static_assert(sizeof(Score) == sizeof(u32));
