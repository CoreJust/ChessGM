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
#include <bit>
#include <string_view>
#include <ostream>

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#include <intrin.h>
#include <nmmintrin.h>
#include <immintrin.h>
#define ENABLE_INTRINSICS
#else
#error "This compiler is ill-supported. Either use Intel (or MSVC) or try to remove this line and compile the code."
#endif

#include "Utils/Types.h"
#include "Defs.h"

/*
*	BitBoard(.h/.cpp) contains the BitBoard type and the functions to work with it.
* 
*	A bit board allows to represent a board as a single number
*	and apply quick bit operations to it. But such board can have only
*	2 values for a single square.
*/

// BB_FOR_EACH allows to go through all the squares in the BitBoard
#define BB_FOR_EACH(sq, bb)\
	if (bb)\
	for (Square sq; bb && (sq = bb.pop(), true);)

class BitBoard final {
public:
	// For the magic bitboards usage for quick move computation
	// Based on the magics from stockfish
	struct MagicBitBoards final {
		u64 mask, *attacks;
#ifndef ENABLE_INTRINSICS
		u64 magic;
		u32 shift;
#endif

		u32 computeIndex(const BitBoard occ) const noexcept {
#ifdef ENABLE_INTRINSICS
			return _pext_u64(occ, mask);
#else
			return ((occ & mask) * magic) >> shift;
#endif
		}
	};

public:
	constexpr inline static u64 EMPTY = 0;
	constexpr inline static u64 FILE_A = 0x0101010101010101;
	constexpr inline static u64 RANK_1 = 0xff;

private:
	u64 m_value;

public:
	// [square][direction]
	// Contains all the bits from the square in the direction
	static u64 s_directionBits[Square::VALUES_COUNT][Direction::VALUES_COUNT];

	// [file]
	// Contains all the bits of the files adjacent to the given
	static u64 s_adjacentFiles[File::VALUES_COUNT];

	// [square][square]
	// Contains the bits between 2 squares if they are on the same line, otherwise 0
	static u64 s_betweenBits[Square::VALUES_COUNT][Square::VALUES_COUNT];

	// [square][square]
	// Contains the bits between on the line that is formed by the squares (if there is)
	static u64 s_alignedBits[Square::VALUES_COUNT][Square::VALUES_COUNT];

	// [pawn color][square]
	// Contains a bitboard of pawn attacks from the given square
	static u64 s_pawnAttacks[Color::VALUES_COUNT][Square::VALUES_COUNT];

	// [piece type][square]
	// Contains a bitboard of attacks of a piece on an empty board (all but pawns)
	static u64 s_pieceAttacks[PieceType::VALUES_COUNT][Square::VALUES_COUNT];

	// [color][castle]
	// Contains the squares between the king and the rook
	static u64 s_castlingInternalSquares[Color::VALUES_COUNT][Castle::VALUES_COUNT];

	// [square]
	// Contains the magic bit board for the square for a bishop
	static MagicBitBoards s_bishopMagic[Square::VALUES_COUNT];

	// [square]
	// Contains the magic bit board for the square for a rook
	static MagicBitBoards s_rookMagic[Square::VALUES_COUNT];

public:
	INLINE constexpr BitBoard() noexcept : m_value(0) { }
	INLINE constexpr BitBoard(const u64 val) noexcept : m_value(val) { }

public:
	// Statics

	// It also calls subsequent initializations of types defined in Defs.h
	static void init() noexcept;

	// Based on stockfish
	static void initMagicBitBoards(const PieceType pt, BitBoard* table, MagicBitBoards* magics) noexcept;

	static BitBoard slidingAttack(const PieceType pt, const Square sq, const BitBoard occupied) noexcept;

	CM_PURE constexpr static BitBoard fromFile(const File file) noexcept {
		high_assert(file < 8);

		return BitBoard(FILE_A << file);
	}

	CM_PURE constexpr static BitBoard fromRank(const Rank rank) noexcept {
		high_assert(rank < 8);

		return BitBoard(RANK_1 << (rank << 3));
	}

	CM_PURE constexpr static BitBoard fromSquare(const Square square) noexcept {
		high_assert(square < 64);

		return BitBoard(1ull << square);
	}

	template<size_t Size>
	CM_PURE consteval static BitBoard fromSquares(const Square(&squares)[Size]) noexcept {
		BitBoard result = BitBoard::EMPTY;
		for (Square sq : squares) {
			high_assert(sq < 64);

			result.m_value |= (1ull << sq);
		}

		return result;
	}

public:
	// Methods

	CM_PURE constexpr bool test(const Square pos) const noexcept {
		high_assert(pos < 64);

		return m_value & (1ull << pos);
	}

	CM_PURE constexpr bool test(const File file, const Rank rank) const noexcept {
		high_assert(file < 8 && rank < 8);

		return m_value & ((1ull << file) << (rank << 3));
	}

	INLINE constexpr void set(const Square pos) noexcept {
		high_assert(pos < 64);

		m_value |= (1ull << pos);
	}

	INLINE constexpr void set(const File file, const Rank rank) noexcept {
		high_assert(file < 8 && rank < 8);

		m_value |= ((1ull << file) << (rank << 3));
	}

	INLINE constexpr void clear(const Square pos) noexcept {
		high_assert(pos < 64);

		m_value &= ~(1ull << pos);
	}

	INLINE constexpr void clear(const File file, const Rank rank) noexcept {
		high_assert(file < 8 && rank < 8);

		m_value &= ~((1ull << file) << (rank << 3));
	}

	INLINE constexpr void swap(const Square sq) noexcept {
		high_assert(sq < 64);

		m_value ^= (1ull << sq);
	}

	INLINE constexpr void move(const Square from, const Square to) noexcept {
		high_assert(from < 64 && to < 64 && from != to);

		m_value ^= (1ull << from) | (1ull << to);
	}

	// Number of set bits
	CM_PURE constexpr i32 popcnt() const noexcept {
		return std::popcount(m_value);
	}

	// Returns the last bit index and erases it
	INLINE constexpr Square pop() noexcept {
		high_assert(m_value != 0);

		if (std::is_constant_evaluated()) {
			Square result = lsb();
			m_value &= (m_value - 1);

			return result;
		} else {
#ifdef ENABLE_INTRINSICS
			unsigned long square;
			_BitScanForward64(&square, m_value);
			m_value &= (m_value - 1);

			return Square(square);
#else // Highly inefficient
			Square result = lsb();
			m_value &= (m_value - 1);

			return result;
#endif
		}
	}

	// Returns the index of the least significant bit
	CM_PURE constexpr Square lsb() const noexcept {
		high_assert(m_value != 0);

		if (std::is_constant_evaluated()) {
			u8 result = 0;
			u64 value = m_value;
			while ((value & 1) == 0) {
				value >>= 1;
				result++;
			}

			return Square(result);
		} else {
#ifdef ENABLE_INTRINSICS
			unsigned long square;
			_BitScanForward64(&square, m_value);

			return Square(square);
#else // Highly inefficient
			u8 result = 0;
			u64 value = m_value;
			while ((value & 1) == 0) {
				value >>= 1;
				result++;
			}

			return Square(result);
#endif
		}
	}

	// Returns the index of the most significant bit
	CM_PURE constexpr Square msb() const noexcept {
		high_assert(m_value != 0);

		if (std::is_constant_evaluated()) {
			u8 result = 0;
			u64 value = m_value;
			while (value & ~1ull) {
				value >>= 1;
				result++;
			}

			return Square(result);
		} else {
#ifdef ENABLE_INTRINSICS
			unsigned long square;
			_BitScanReverse64(&square, m_value);

			return Square(square);
#else // Highly inefficient
			u8 result = 0;
			u64 value = m_value;
			while (value & ~1ull) {
				value >>= 1;
				result++;
			}

			return Square(result);
#endif
		}
	}

	// Checks if there is more than one bit
	CM_PURE constexpr bool hasMoreThanOne() const noexcept {
		return m_value & (m_value - 1);
	}

	CM_PURE constexpr BitBoard b_and(const BitBoard other) const noexcept {
		return m_value & other;
	}

	CM_PURE constexpr BitBoard b_or(const BitBoard other) const noexcept {
		return m_value | other;
	}

	CM_PURE constexpr BitBoard b_xor(const BitBoard other) const noexcept {
		return m_value ^ other;
	}

	CM_PURE constexpr BitBoard b_not() const noexcept {
		return ~m_value;
	}

	CM_PURE constexpr BitBoard shift(const Direction dir) const noexcept {
		switch (dir) {
			case Direction::UP: return BitBoard(m_value << 8);
			case Direction::DOWN: return BitBoard(m_value >> 8);
			case Direction::LEFT: return BitBoard(m_value << 1).b_and(~BitBoard::fromFile(File::A));
			case Direction::RIGHT: return BitBoard(m_value >> 1).b_and(~BitBoard::fromFile(File::H));
			case Direction::UPRIGHT: return BitBoard(m_value << 7).b_and(~BitBoard::fromFile(File::H));
			case Direction::UPLEFT: return BitBoard(m_value << 9).b_and(~BitBoard::fromFile(File::A));
			case Direction::DOWNRIGHT: return BitBoard(m_value >> 9).b_and(~BitBoard::fromFile(File::H));
			case Direction::DOWNLEFT: return BitBoard(m_value >> 7).b_and(~BitBoard::fromFile(File::A));
		default: return 0;
		}
	}

	// Checks if the squares are on the same line
	CM_PURE static bool areAligned(const Square a, const Square b, const Square c) noexcept {
		high_assert(a < 64 && b < 64 && c < 64);

		return s_alignedBits[a][b] & (1ull << c);
	}

	std::string_view toString() const noexcept;

	
	// Getters

	CM_PURE static BitBoard alignedBits(const Square a, const Square b) noexcept {
		high_assert(a < 64 && b < 64);

		return s_alignedBits[a][b];
	}

	CM_PURE static BitBoard betweenBits(const Square a, const Square b) noexcept {
		high_assert(a < 64 && b < 64);

		return s_betweenBits[a][b];
	}

	CM_PURE static BitBoard castlingInternalSquares(const Color color, const Castle castle) noexcept {
		return s_castlingInternalSquares[color][castle];
	}


	// Chess related methods

	template<Color::Value Side>
	CM_PURE constexpr BitBoard pawnAttackedSquares() const noexcept {
		if constexpr (Side == Color::WHITE) {
			return shift(Direction::UPLEFT) | shift(Direction::UPRIGHT);
		} else { // Black
			return shift(Direction::DOWNLEFT) | shift(Direction::DOWNRIGHT);
		}
	}

	CM_PURE constexpr static BitBoard pawnAttacks(const Color color, const Square sq) noexcept {
		return s_pawnAttacks[color][sq];
	}

	template<PieceType::Value PT>
	CM_PURE static BitBoard pseudoAttacks(const Square sq) noexcept {
		static_assert(PT != PieceType::PAWN && PT != PieceType::NONE);

		return s_pieceAttacks[PT][sq];
	}

	CM_PURE static BitBoard attacksOf(const PieceType pt, const Square sq, const BitBoard occ) noexcept {
		high_assert(pt != PieceType::PAWN && pt != PieceType::NONE);

		switch (pt) {
			case PieceType::BISHOP: return s_bishopMagic[sq].attacks[s_bishopMagic[sq].computeIndex(occ)];
			case PieceType::ROOK: return s_rookMagic[sq].attacks[s_rookMagic[sq].computeIndex(occ)];
			case PieceType::QUEEN: return attacksOf(PieceType::ROOK, sq, occ).b_or(attacksOf(PieceType::BISHOP, sq, occ));
		default: return s_pieceAttacks[pt][sq];
		}
	}

	CM_PURE static BitBoard bishopAttackedSquares(const BitBoard blockers, const BitBoard friendlyPieces, const Square pos) noexcept {
		BitBoard rays = s_directionBits[pos][Direction::UPRIGHT];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).lsb()][Direction::UPRIGHT];

		BitBoard squares_attacked = rays;

		rays = s_directionBits[pos][Direction::UPLEFT];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).lsb()][Direction::UPLEFT];
		squares_attacked |= rays;

		rays = s_directionBits[pos][Direction::DOWNRIGHT];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).msb()][Direction::DOWNRIGHT];
		squares_attacked |= rays;

		rays = s_directionBits[pos][Direction::DOWNLEFT];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).msb()][Direction::DOWNLEFT];

		return (rays | squares_attacked) & ~friendlyPieces;
	}

	CM_PURE static BitBoard rookAttackedSquares(const BitBoard blockers, const BitBoard friendlyPieces, const Square pos) noexcept {
		BitBoard rays = s_directionBits[pos][Direction::UP];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).lsb()][Direction::UP];

		BitBoard squares_attacked = rays;

		rays = s_directionBits[pos][Direction::RIGHT];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).lsb()][Direction::RIGHT];
		squares_attacked |= rays;

		rays = s_directionBits[pos][Direction::LEFT];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).msb()][Direction::LEFT];
		squares_attacked |= rays;

		rays = s_directionBits[pos][Direction::DOWN];
		if (rays & blockers) rays ^= s_directionBits[rays.b_and(blockers).msb()][Direction::DOWN];

		return (rays | squares_attacked) & ~friendlyPieces;
	}

	CM_PURE static BitBoard queenAttackedSquares(const BitBoard blockers, const BitBoard friendlyPieces, const Square pos) noexcept {
		return bishopAttackedSquares(blockers, friendlyPieces, pos) | rookAttackedSquares(blockers, friendlyPieces, pos);
	}

public:
	// Operators
	CM_PURE constexpr operator u64() const noexcept {
		return m_value;
	}

	INLINE constexpr BitBoard& operator|=(const BitBoard other) noexcept {
		m_value |= other;
		return *this;
	}

	INLINE constexpr BitBoard& operator&=(const BitBoard other) noexcept {
		m_value &= other;
		return *this;
	}

	INLINE constexpr BitBoard& operator^=(const BitBoard other) noexcept {
		m_value ^= other;
		return *this;
	}

	friend std::ostream& operator<<(std::ostream& out, const BitBoard bb);
};