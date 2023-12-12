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
#include <string>
#include "Utils/EnumWrap.h"

/*
*	Defs.h contains some definitions and chess-related types.
*	
*	Types defined here:
*		1) Color - the color of a piece/the side of a player
*		2) PieceType - enumeration of pieces independently from their color
*		3) Piece - PieceType with a Color in one type
*		4) File and Rank - the file and rank of the chessboard correspondingly
*		5) Square - a single square, file and rank in one type
*		6) Direction - sefl-obvious, one of the 8 directions
*		7) Castle - the castling side
*		8) GameResult - the outcome of the game
* 
*	Note:
*		makeRelativeXXX(Color, XXX) static methods return the unchanged
*		value for white and the opposite for black.
*/

#undef max
#undef min

WRAPPED_ENUM_BEGIN(Color, u8)
	BLACK = 0,
	WHITE,
WRAPPED_ENUM_IMPL
	CM_PURE constexpr static Color fromFENChar(const char ch) noexcept {
		assert(ch == 'w' || ch == 'b');

		return ch == 'w' ? Color::WHITE : Color::BLACK;
	}

	CM_PURE constexpr Color getOpposite() const noexcept {
		return Color(m_value ^ 1);
	}
	
	constexpr std::string_view toString() const noexcept {
		const char* names[Value::VALUES_COUNT] = {
			"black",
			"white"
		};

		high_assert(m_value < std::size(names));

		return names[m_value];
	}
WRAPPED_ENUM_END(Color, u8)



WRAPPED_ENUM_BEGIN(PieceType, u8)
	NONE = 0,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
WRAPPED_ENUM_IMPL
	constexpr std::string_view toString() const noexcept {
		const char* names[Value::VALUES_COUNT] = {
			"none",
			"pawn",
			"knight",
			"bishop",
			"rook",
			"queen",
			"king",
		};

		high_assert(m_value < std::size(names));

		return names[m_value];
	}

	constexpr char toChar() const noexcept {
		high_assert(m_value < std::size("upnbrqk"));

		return "upnbrqk"[m_value];
	}
WRAPPED_ENUM_END(PieceType, u8)



WRAPPED_ENUM_BEGIN(Piece, u8)
	NONE = 0,
	PAWN_BLACK = 2,
	PAWN_WHITE,
	KNIGHT_BLACK,
	KNIGHT_WHITE,
	BISHOP_BLACK,
	BISHOP_WHITE,
	ROOK_BLACK,
	ROOK_WHITE,
	QUEEN_BLACK,
	QUEEN_WHITE,
	KING_BLACK,
	KING_WHITE,
WRAPPED_ENUM_IMPL
	INLINE constexpr Piece(const Color color, const PieceType pt) noexcept : EnumWrap<u8>(u8(color) | (u8(pt) << 1)) { }

	CM_PURE constexpr static Piece fromFENChar(const char ch) noexcept {
		switch (ch) {
			case 'P': return PAWN_WHITE;
			case 'N': return KNIGHT_WHITE;
			case 'B': return BISHOP_WHITE;
			case 'R': return ROOK_WHITE;
			case 'Q': return QUEEN_WHITE;
			case 'K': return KING_WHITE;
			case 'p': return PAWN_BLACK;
			case 'n': return KNIGHT_BLACK;
			case 'b': return BISHOP_BLACK;
			case 'r': return ROOK_BLACK;
			case 'q': return QUEEN_BLACK;
			case 'k': return KING_BLACK;
		default: 
			assert(false && "unreachable"); 
			return NONE;
		}
	}

	CM_PURE constexpr Color getColor() const noexcept {
		return Color::Value(m_value & 1);
	}

	CM_PURE constexpr PieceType getType() const noexcept {
		return PieceType::Value(m_value >> 1);
	}

	constexpr char toChar() const noexcept {
		return "  pPnNbBrRqQkK "[m_value];
	}
WRAPPED_ENUM_END(Piece, u8)



WRAPPED_ENUM_BEGIN(File, u8)
	A = 0,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
WRAPPED_ENUM_IMPL
	CM_PURE constexpr static File fromFENChar(const char ch) noexcept {
		assert(ch >= 'a' && ch <= 'h');

		return File(ch - 'a');
	}

	CM_PURE constexpr static u8 distance(const File a, const File b) noexcept {
		if (std::is_constant_evaluated()) {
			return a < b ? (b - a) : (a - b);
		} else {
			return std::abs(a - b);
		}
	}

	constexpr std::string_view toString() const noexcept {
		const char* names[Value::VALUES_COUNT] = {
			"a",
			"b",
			"c",
			"d",
			"e",
			"f",
			"g",
			"h"
		};

		high_assert(m_value < std::size(names));

		return names[m_value];
	}
WRAPPED_ENUM_END(File, u8)



WRAPPED_ENUM_BEGIN(Rank, u8)
	R1 = 0,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
WRAPPED_ENUM_IMPL
	CM_PURE constexpr static Rank fromFENChar(const char ch) noexcept {
		assert(ch >= '1' && ch <= '8');

		return Rank(ch - '1');
	}

	CM_PURE constexpr static u8 distance(const Rank a, const Rank b) noexcept {
		if (std::is_constant_evaluated()) {
			return a < b ? (b - a) : (a - b);
		} else {
			return std::abs(a - b);
		}
	}

	CM_PURE constexpr static Rank makeRelativeRank(const Color color, const Rank rank) noexcept {
		return Rank(rank ^ (0x7 * color.getOpposite()));
	}

	CM_PURE constexpr Rank getOpposite() const noexcept {
		return Rank(m_value ^ 0x7);
	}

	constexpr std::string_view toString() const noexcept {
		const char* names[Value::VALUES_COUNT] = {
			"1",
			"2",
			"3",
			"4",
			"5",
			"6",
			"7",
			"8"
		};

		high_assert(m_value < std::size(names));

		return names[m_value];
	}
WRAPPED_ENUM_END(Rank, u8)



WRAPPED_ENUM_BEGIN(Direction, u8)
	UP = 0,
	LEFT,
	UPLEFT,
	UPRIGHT,
	DOWNLEFT,
	DOWNRIGHT,
	RIGHT,
	DOWN,
WRAPPED_ENUM_IMPL_WITH(NO_DIRECTION)
	CM_PURE constexpr static Direction makeRelativeDirection(const Color color, const Direction dir) noexcept {
		return color == Color::BLACK ? dir.getOpposite() : dir;
	}

	CM_PURE constexpr Direction getOpposite() const noexcept {
		return Direction(0x7 & ~m_value);
	}
WRAPPED_ENUM_END_WITH(Direction, u8, NO_DIRECTION)



WRAPPED_ENUM_BEGIN(Square, u8)
	A1 = 0, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
WRAPPED_ENUM_IMPL_WITH(NO_POS)
private:
	inline static u8 s_distance[VALUES_COUNT][VALUES_COUNT];

public:
	INLINE constexpr Square(const File file, const Rank rank) noexcept : EnumWrap<u8>(file | u8(rank << 3)) { }

	INLINE static void init() noexcept {
		for (u8 a = 0; a < 64; a++) {
			const Square sqa(a);
			for (u8 b = 0; b < 64; b++) {
				const Square sqb(b);
				s_distance[a][b] = std::max(Rank::distance(sqa.getRank(), sqb.getRank()), File::distance(sqa.getFile(), sqb.getFile()));
			}
		}
	}

	CM_PURE static u8 distance(const Square a, const Square b) noexcept {
		return s_distance[a][b];
	}

	// For example: 'e', '8'
	CM_PURE constexpr static Square fromChars(const char f, const char r) noexcept {
		return Square(File::fromFENChar(tolower(f)), Rank::fromFENChar(r));
	}

	// Relative in rank
	CM_PURE constexpr static Square makeRelativeSquare(const Color color, const Square sq) noexcept {
		return Square(sq ^ (0x38 * color.getOpposite()));
	}

	// Inverses the rank
	CM_PURE constexpr Square getOpposite() const noexcept {
		return Square(m_value ^ 0x38);
	}

	CM_PURE constexpr Square mirrorByFile() const noexcept {
		return Square(m_value ^ 0x7);
	}
	
	// Advances the square for <value> positions forward
	CM_PURE constexpr Square forward(const u8 value = 1) const noexcept {
		return Square(m_value + value);
	}

	// Advances the square for <value> positions backward
	CM_PURE constexpr Square backward(const u8 value = 1) const noexcept {
		return Square(m_value - value);
	}

	CM_PURE constexpr Square shift(const Direction dir) const noexcept {
		switch (dir) {
			case Direction::UP: return Square(m_value + 8);
			case Direction::DOWN: return Square(m_value - 8);
			case Direction::LEFT: return Square(m_value + 1);
			case Direction::RIGHT: return Square(m_value - 1);
			case Direction::UPRIGHT: return Square(m_value + 7);
			case Direction::UPLEFT: return Square(m_value + 9);
			case Direction::DOWNRIGHT: return Square(m_value - 9);
			case Direction::DOWNLEFT: return Square(m_value - 7);
		default: return NO_POS;
		}
	}

	CM_PURE constexpr Rank getRank() const noexcept {
		return Rank::Value(m_value >> 3);
	}

	CM_PURE constexpr File getFile() const noexcept {
		return File::Value(m_value & 7);
	}

	CM_PURE constexpr Square inverseRank() const noexcept {
		return Value(m_value ^ 0x38);
	}

	CM_PURE constexpr Square inverseFile() const noexcept {
		return Value(m_value ^ 0x7);
	}

	inline std::string toString() const noexcept {
		return std::string(getFile().toString()).append(getRank().toString());
	}
WRAPPED_ENUM_END_WITH(Square, u8, NO_POS)



WRAPPED_ENUM_BEGIN(Castle, u8)
	QUEEN_CASTLE = 0,
	KING_CASTLE,
	BOTH_CASTLE,

	CASTLE_DONE, // Flag for the situation when the king already castled (and thus the castling is not possible anymore)
WRAPPED_ENUM_IMPL
private:
	inline static u8 s_castleRightMasks[Square::VALUES_COUNT];

public:

	INLINE static void init() noexcept {
		const u8 castleChange[Square::VALUES_COUNT] = {
			4, 0, 0, 0, 12, 0, 0, 8,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 3, 0, 0, 2
		};

		for (Square sq : Square::iter()) {
			s_castleRightMasks[sq] = ~castleChange[sq];
		}
	}

	// Allows to contain all the information on castling in a single byte
	// It is guaranteed that for both colors:
	//		1) Mask for BOTH_CASTLE = mask for QUEEN_CASTLE | mask for KING_CASTLE
	//		2) Mask for CASTLE_DONE, QUEEN_CASTLE, and KING_CASTLE do not have common bits
	CM_PURE constexpr static u8 getBitMaskFor(const Castle castle, const Color color) noexcept {
		const u8 MASKS[Color::VALUES_COUNT][VALUES_COUNT] = {
			{ // Black
				0x1,
				0x2,
				0x3,
				0x10
			}, { // White
				0x4,
				0x8,
				0xc,
				0x20
			}
		};

		return MASKS[color][castle];
	}

	CM_PURE constexpr static bool hasCastleRight(const u8 rights, const Castle castle, const Color color) noexcept {
		const u8 mask = getBitMaskFor(castle, color);
		return (rights & mask) == mask;
	}

	CM_PURE constexpr static bool hasAnyCastleRight(const u8 rights) noexcept {
		const u8 mask = getBitMaskFor(BOTH_CASTLE, Color::BLACK) | getBitMaskFor(BOTH_CASTLE, Color::WHITE);
		return bool(rights & mask);
	}

	CM_PURE constexpr static u8 getCastleChangeMask(const Square sq) noexcept {
		high_assert(sq < 64);

		return s_castleRightMasks[sq];
	}

	CM_PURE constexpr static u8 bitMaskFromFENChar(const char ch) noexcept {
		switch (ch) {
			case 'K': return getBitMaskFor(Castle::KING_CASTLE,  Color::WHITE);
			case 'Q': return getBitMaskFor(Castle::QUEEN_CASTLE, Color::WHITE);
			case 'k': return getBitMaskFor(Castle::KING_CASTLE,  Color::BLACK);
			case 'q': return getBitMaskFor(Castle::QUEEN_CASTLE, Color::BLACK);
		default: unreachable(); return 0;
		}
	}

	constexpr static char toFENChar(const Castle castle, const Color color) noexcept {
		const char CHARS[Color::VALUES_COUNT][VALUES_COUNT] = { { 'q', 'k', ' ', ' ' }, { 'Q', 'K', ' ', ' ' } };
		return CHARS[color][castle];
	}
WRAPPED_ENUM_END(Castle, u8)




WRAPPED_ENUM_BEGIN(GameResult, u8)
	NONE = 0,
	WHITE_WON,
	DRAW,
	BLACK_WON,
WRAPPED_ENUM_IMPL
WRAPPED_ENUM_END(GameResult, u8)