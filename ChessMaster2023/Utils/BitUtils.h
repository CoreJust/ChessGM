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

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#include <intrin.h>
#include <nmmintrin.h>
#include <immintrin.h>
#define ENABLE_INTRINSICS
#endif

#include "Types.h"

/*
*	BitUtils.h contains several functions for bit manipulations.
*/

namespace bit_utils {
	// Parallel extract function
	// Extracts from value only those bits that are set in mask
	// and remaps them to packed number
	// For example (for u8): parallelExtract(value = 0b11010010, mask = 0b10101010) -> 0b1_0_0_1_
	CM_PURE constexpr u64 parallelExtract(u64 value, i64 mask) noexcept {
#ifdef ENABLE_INTRINSICS
		if (std::is_constant_evaluated()) {
			u64 result = 0;
			u64 bit = 1;

			do {
				u64 lsb = mask & -mask;
				mask &= ~lsb;
				result |= bool(value & lsb) * bit;
				bit <<= 1;
			} while (mask);

			return result;
		} else {
			return _pext_u64(value, mask);
		}
#else
		u64 result = 0;
		u64 bit = 1;

		do {
			u64 lsb = mask & -mask;
			mask &= ~lsb;
			result |= bool(value & lsb) * bit;
			bit <<= 1;
		} while (mask);

		return result;
#endif
	}

	// Returns the index of the least significant bit
	CM_PURE constexpr u8 leastSignificantBit(const u64 value) noexcept {
		if (std::is_constant_evaluated()) {
			return static_cast<u8>(std::countr_zero(value));
		} else {
#ifdef ENABLE_INTRINSICS
			unsigned long result;
			_BitScanForward64(&result, value);

			return static_cast<u8>(result);
#elif defined(__GNUC__)
			return static_cast<u8>(__builtin_ctzll(value));
#else
			return static_cast<u8>(std::countr_zero(value));
#endif
		}
	}

	// Returns the index of the most significant bit
	CM_PURE constexpr u8 mostSignificantBit(const u64 value) noexcept {
		if (std::is_constant_evaluated()) {
			return static_cast<u8>(63 - std::countl_zero(value));
		} else {
#ifdef ENABLE_INTRINSICS
			unsigned long result;
			_BitScanReverse64(&result, value);

			return static_cast<u8>(result);
#elif defined(__GNUC__)
			return static_cast<u8>(63 ^ __builtin_clzll(value));
#else
			return static_cast<u8>(63 - std::countl_zero(value));
#endif
		}
	}

	// Returns the number of bits set to 1
	CM_PURE constexpr u8 popCount(u64 value) noexcept {
		if (std::is_constant_evaluated()) {
			return static_cast<u8>(std::popcount(value));
		} else {
#ifdef ENABLE_INTRINSICS
			return static_cast<u8>(_mm_popcnt_u64(value));
#elif defined(__GNUC__)
			return static_cast<u8>(__builtin_popcountll(value));
#else
			return static_cast<u8>(std::popcount(value));
#endif
		}
	}
}
