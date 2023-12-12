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

#if defined(__cpp_lib_unreachable)
#include <utility> // for std::unreachable()
#endif

#include "HighAssert.h"

/*
*	Macro.h, just as the name implies, contains some macro definitions related to
*	the language and compiler version.
*/

#undef CM_PURE
#undef INLINE
#undef NO_UNIQUE_ADDRESS
#undef LIKELY
#undef UNLIKELY
#undef LANG_VERSION
#undef max

// LANG_VERSION - the version of c++
#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#define LANG_VERSION _MSVC_LANG
#else
#define LANG_VERSION __cplusplus
#endif

// Self-explanatory
#if LANG_VERSION < 202002L
#error "C++ language version must be c++20 or higher"
#endif

// Define unreachable, must be either clang, gcc, intel, or msvc
#if defined(__cpp_lib_unreachable)
#define unreachable() std::unreachable()
#elif defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER)
#define unreachable() __builtin_unreachable()
#ifdef __GNUC__
#error "GNU compiler is ill-supported. Use MSVC or Clang, or remove this error and try to modify the code"
#endif
#elif defined(_MSC_VER)
#define unreachable() __assume(false)
#else
#error "Unsupported compiler"
#endif

#if defined(_MSC_VER)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

// If [[likely]] and [[unlikely]] is not supported - delete them
#if !defined(__has_cpp_attribute)
#define LIKELY
#define UNLIKELY
#elif !(__has_cpp_attribute(likely) && __has_cpp_attribute(unlikely))
#define LIKELY
#define UNLIKELY
#else
#define LIKELY [[likely]]
#define UNLIKELY [[unlikely]]
#endif

#if defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE __attribute__((always_inline))
#endif

#define CM_PURE [[nodiscard]] INLINE