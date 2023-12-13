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

#include "BitBoard.h"
#include <cstring>


u64 BitBoard::s_directionBits[Square::VALUES_COUNT][Direction::VALUES_COUNT];
u64 BitBoard::s_adjacentFiles[File::VALUES_COUNT];
u64 BitBoard::s_betweenBits[Square::VALUES_COUNT][Square::VALUES_COUNT];
u64 BitBoard::s_alignedBits[Square::VALUES_COUNT][Square::VALUES_COUNT];
u64 BitBoard::s_pawnAttacks[Color::VALUES_COUNT][Square::VALUES_COUNT];
u64 BitBoard::s_pieceAttacks[PieceType::VALUES_COUNT][Square::VALUES_COUNT];
u64 BitBoard::s_castlingInternalSquares[Color::VALUES_COUNT][Castle::VALUES_COUNT];
BitBoard::MagicBitBoards BitBoard::s_bishopMagic[Square::VALUES_COUNT];
BitBoard::MagicBitBoards BitBoard::s_rookMagic[Square::VALUES_COUNT];


void BitBoard::init() noexcept {
	static BitBoard s_rookTable[0x19000];
	static BitBoard s_bishopTable[0x1480];

	memset(s_rookTable, 0, sizeof(s_rookTable));
	memset(s_bishopTable, 0, sizeof(s_bishopTable));

	memset(s_directionBits, 0, sizeof(s_directionBits));
	memset(s_adjacentFiles, 0, sizeof(s_adjacentFiles));
	memset(s_betweenBits, 0, sizeof(s_betweenBits));
	memset(s_alignedBits, 0, sizeof(s_alignedBits));
	memset(s_pawnAttacks, 0, sizeof(s_pawnAttacks));
	memset(s_pieceAttacks, 0, sizeof(s_pieceAttacks));
	memset(s_castlingInternalSquares, 0, sizeof(s_castlingInternalSquares));

	Square::init();
	Castle::init();

	initMagicBitBoards(PieceType::ROOK, s_rookTable, s_rookMagic);
	initMagicBitBoards(PieceType::BISHOP, s_bishopTable, s_bishopMagic);

	for (auto i : Square::iter()) {
		for (i32 j = i + 8; j < 64; j += 8) s_directionBits[i][Direction::UP] |= (1ull << j);
		for (i32 j = i - 8; j >= 0; j -= 8) s_directionBits[i][Direction::DOWN] |= (1ull << j);
		for (i32 j = i - 1; (j & 7) < 7; j--) s_directionBits[i][Direction::LEFT] |= (1ull << j);
		for (i32 j = i + 1; (j & 7) > 0; j++) s_directionBits[i][Direction::RIGHT] |= (1ull << j);

		for (i32 j = i + 9; j < 64 && (j & 7) > 0; j += 9) s_directionBits[i][Direction::UPRIGHT] |= (1ull << j);
		for (i32 j = i - 7; j >= 0 && (j & 7) > 0; j -= 7) s_directionBits[i][Direction::DOWNRIGHT] |= (1ull << j);
		for (i32 j = i - 9; j >= 0 && (j & 7) < 7; j -= 9) s_directionBits[i][Direction::DOWNLEFT] |= (1ull << j);
		for (i32 j = i + 7; j < 64 && (j & 7) < 7; j += 7) s_directionBits[i][Direction::UPLEFT] |= (1ull << j);
	}

	for (auto file : File::iter()) {
		s_adjacentFiles[file] = BitBoard::fromFile(file).shift(Direction::RIGHT) | BitBoard::fromFile(file).shift(Direction::LEFT);
	}

	for (auto i : Square::iter()) {
		const BitBoard sqBB = BitBoard::fromSquare(i);
		s_pawnAttacks[Color::WHITE][i] = sqBB.pawnAttackedSquares<Color::WHITE>();
		s_pawnAttacks[Color::BLACK][i] = sqBB.pawnAttackedSquares<Color::BLACK>();

		for (auto dir : Direction::iter()) {
			s_pieceAttacks[PieceType::KING][i] |= sqBB.shift(dir);
		}

		s_pieceAttacks[PieceType::KNIGHT][i] =
			sqBB.shift(Direction::UP).shift(Direction::UPLEFT) |
			sqBB.shift(Direction::UP).shift(Direction::UPRIGHT) |
			sqBB.shift(Direction::LEFT).shift(Direction::UPLEFT) |
			sqBB.shift(Direction::LEFT).shift(Direction::DOWNLEFT) |
			sqBB.shift(Direction::RIGHT).shift(Direction::UPRIGHT) |
			sqBB.shift(Direction::RIGHT).shift(Direction::DOWNRIGHT) |
			sqBB.shift(Direction::DOWN).shift(Direction::DOWNLEFT) |
			sqBB.shift(Direction::DOWN).shift(Direction::DOWNRIGHT);

		s_pieceAttacks[PieceType::BISHOP][i] = attacksOf(PieceType::BISHOP, i, BitBoard::EMPTY);
		s_pieceAttacks[PieceType::ROOK][i] = attacksOf(PieceType::ROOK, i, BitBoard::EMPTY);
		s_pieceAttacks[PieceType::QUEEN][i] = BitBoard(s_pieceAttacks[PieceType::BISHOP][i]).b_or(s_pieceAttacks[PieceType::ROOK][i]);

		for (Direction dir : Direction::iter()) {
			BitBoard bb = s_directionBits[i][dir];
			BB_FOR_EACH(sq, bb) {
				s_betweenBits[i][sq] = s_directionBits[i][dir] & ~s_directionBits[sq][dir];
				s_alignedBits[i][sq] = s_directionBits[i][dir] | s_directionBits[i][dir.getOpposite()] | BitBoard::fromSquare(i);
			}
		}

		for (Square to : Square::iter()) {
			s_betweenBits[i][to] |= (1ull << to);
		}
	}

	s_castlingInternalSquares[Color::WHITE][Castle::QUEEN_CASTLE] = BitBoard::fromSquares({ Square::B1, Square::C1, Square::D1 });
	s_castlingInternalSquares[Color::WHITE][Castle::KING_CASTLE] = BitBoard::fromSquares({ Square::F1, Square::G1 });
	s_castlingInternalSquares[Color::WHITE][Castle::BOTH_CASTLE] = BitBoard::fromSquares({ Square::B1, Square::C1, Square::D1, Square::F1, Square::G1 });

	s_castlingInternalSquares[Color::BLACK][Castle::QUEEN_CASTLE] = BitBoard::fromSquares({ Square::B8, Square::C8, Square::D8 });
	s_castlingInternalSquares[Color::BLACK][Castle::KING_CASTLE] = BitBoard::fromSquares({ Square::F8, Square::G8 });
	s_castlingInternalSquares[Color::BLACK][Castle::BOTH_CASTLE] = BitBoard::fromSquares({ Square::B8, Square::C8, Square::D8, Square::F8, Square::G8 });
}

void BitBoard::initMagicBitBoards(const PieceType pt, BitBoard* table, MagicBitBoards* magics) noexcept {
	BitBoard occupancy[4096], reference[4096], edges, b;
	i32      cnt = 0, size = 0;

	for (Square s : Square::iter()) {
		edges = ((BitBoard::RANK_1 | BitBoard::fromRank(Rank::R8)) & ~BitBoard::fromRank(Square(s).getRank()))
			| ((BitBoard::FILE_A | BitBoard::fromFile(File::H)) & ~BitBoard::fromFile(Square(s).getFile()));

		MagicBitBoards& m = magics[s];
		m.mask = slidingAttack(pt, s, 0) & ~edges;
		m.attacks = (u64*)(s == Square::A1 ? table : (BitBoard*)magics[s - 1].attacks + size);

		b = size = 0;
		do {
			occupancy[size] = b;
			reference[size] = slidingAttack(pt, s, b);
			m.attacks[bit_utils::parallelExtract(b, m.mask)] = reference[size];

			size++;
			b = (b - m.mask) & m.mask;
		} while (b);
	}
}

BitBoard BitBoard::slidingAttack(const PieceType pt, const Square sq, const BitBoard occupied) noexcept {
	BitBoard result = BitBoard::EMPTY;
	if (pt == PieceType::ROOK) {
		for (Direction dir : { Direction::UP, Direction::DOWN, Direction::RIGHT, Direction::LEFT }) {
			BitBoard bb = BitBoard::fromSquare(sq);
			while (bb = bb.shift(dir)) {
				result |= bb;

				if (occupied.b_and(bb)) break;
			}
		}
	} else if (pt == PieceType::BISHOP) {
		for (Direction dir : { Direction::UPLEFT, Direction::DOWNLEFT, Direction::UPRIGHT, Direction::DOWNRIGHT }) {
			BitBoard bb = BitBoard::fromSquare(sq);
			while (bb = bb.shift(dir)) {
				result |= bb;

				if (occupied.b_and(bb)) break;
			}
		}
	}

	return result;
}

std::string_view BitBoard::toString() const noexcept {
	static char s_str[73];

	char* ch = s_str;
	for (auto rank : Rank::reverseIter()) {
		for (auto file : File::iter()) {
			*(ch++) = (test(file, rank) ? 'X' : '-');
		}

		*(ch++) = '\n';
	}

	*(ch++) = '\0';

	return s_str;
}

std::ostream& operator<<(std::ostream& out, const BitBoard bb) {
	return out << bb.toString();
}
