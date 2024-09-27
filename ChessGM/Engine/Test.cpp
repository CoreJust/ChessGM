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

#include "Test.h"

#include <chrono>
#include <tuple>

#include "Utils/IO.h"
#include "Chess/BitBoard.h"
#include "Engine/Scores.h"
#include "Engine/Search.h"


///  UTILS FOR TESTS  ///

#define EXPECT_TRUE(expr)\
	if (!(expr)) { \
		io::g_out << io::Color::Red << "Expectation failed, not true: " #expr << " in test " \
		<< testName << std::endl; \
		return false; \
	}

#define EXPECT_EQ(expr1, expr2)\
	if (decltype(expr1) _e1 = (expr1), _e2 = (expr2); _e1 != _e2) { \
		io::g_out << io::Color::Red << "Expectation failed, not equals: " #expr1 " ( = " << _e1 \
		<< ") and " #expr2 " ( = " << _e2 << ") in test " << testName << std::endl; \
		return false; \
	}

template <u32 Id>
bool test();

template<class T>
u32 countValuesOfType() {
	u32 result = 0;
	for ([[maybe_unused]] T t : T::iter()) {
		++result;
	}

	return result;
}


///  CHESS TYPES TEST  ///

template<> bool test<0>() {
	constexpr auto testName = "ChessTypesTest(iterationsTest)";

	EXPECT_EQ(countValuesOfType<Square>(), u32(64));
	EXPECT_EQ(countValuesOfType<File>(), u32(8));
	EXPECT_EQ(countValuesOfType<Rank>(), u32(8));
	EXPECT_EQ(countValuesOfType<Color>(), u32(2));
	EXPECT_EQ(countValuesOfType<Direction>(), u32(8));
	EXPECT_EQ(countValuesOfType<PieceType>(), u32(7)); // none, pawn, knight, bishop, rook, queen, king
	EXPECT_EQ(countValuesOfType<Piece>(), u32(14)); // count of PieceTypes * 2
	EXPECT_EQ(countValuesOfType<Castle>(), u32(4)); // queen/king castles, both sides castle, castle done
	EXPECT_EQ(countValuesOfType<GameResult>(), u32(4)); // none, wins of 2 sides, draw

	return true;
}

template<> bool test<1>() {
	constexpr auto testName = "ChessTypesTest(squareTest)";

	Square sq = Square::fromChars('c', '2');

	EXPECT_EQ(sq.forward(8), Square::C3);
	EXPECT_EQ(sq.backward(8), Square::C1);
	EXPECT_EQ(sq.shift(Direction::UPLEFT), Square::B3);
	EXPECT_EQ(sq.shift(Direction::DOWNRIGHT), Square::D1);
	EXPECT_EQ(sq.getOpposite(), Square::C7);
	EXPECT_EQ(sq.toString(), "c2");

	for (File file : File::iter()) {
		for (Rank rank : Rank::iter()) {
			sq = Square(file, rank);

			EXPECT_EQ(sq.getFile(), file);
			EXPECT_EQ(sq.getRank(), rank);
		}
	}

	return true;
}

template<> bool test<2>() {
	constexpr auto testName = "ChessTypesTest(pieceTest)";

	for (PieceType pt : PieceType::iter()) {
		for (Color color : Color::iter()) {
			Piece piece = Piece(color, pt);

			EXPECT_EQ(piece.getColor(), color);
			EXPECT_EQ(piece.getType(), pt);
		}
	}

	return true;
}


///  BITBOARD TESTS  ///

template<> bool test<3>() {
	constexpr auto testName = "BitBoardTest(fromSquareTest)";

	for (Square sq : Square::iter()) {
		BitBoard b = BitBoard::fromSquare(sq);

		EXPECT_TRUE(b.test(sq));
		EXPECT_EQ(b.lsb(), sq);
		EXPECT_EQ(b.msb(), sq);
		EXPECT_EQ(b.popcnt(), 1);
	}

	return true;
}

template<> bool test<4>() {
	constexpr auto testName = "BitBoardTest(fromRankTest)";

	for (Rank rank : Rank::iter()) {
		BitBoard b = BitBoard::fromRank(rank);

		for (File file : File::iter()) {
			EXPECT_TRUE(b.test(file, rank));
		}

		EXPECT_EQ(b.lsb().getFile(), File::A);
		EXPECT_EQ(b.msb().getFile(), File::H);
	}

	return true;
}

template<> bool test<5>() {
	constexpr auto testName = "BitBoardTest(fromFileTest)";

	for (File file : File::iter()) {
		BitBoard b = BitBoard::fromFile(file);

		for (Rank rank : Rank::iter()) {
			EXPECT_TRUE(b.test(file, rank));
		}

		EXPECT_EQ(b.lsb().getRank(), Rank::R1);
		EXPECT_EQ(b.msb().getRank(), Rank::R8);
	}

	return true;
}


///  BOARD TESTS  ///


const std::string TEST_FENS[] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
	"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
	"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
	"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
	"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
};


template<> bool test<6>() {
	constexpr auto testName = "BoardTest(creationFromTest)";

	for (const auto& fen : TEST_FENS) {
		bool success;
		Board board = Board::fromFEN(fen, success);

		EXPECT_EQ(board.toFEN(), fen);
		EXPECT_TRUE(success);

		for (Square sq : Square::iter()) {
			Piece piece = board[sq];
			
			if (piece != Piece::NONE) {
				EXPECT_TRUE(board.byPiece(piece).test(sq));
				EXPECT_TRUE(board.byPieceType(piece.getType()).test(sq));
				EXPECT_TRUE(board.byColor(piece.getColor()).test(sq));
				EXPECT_EQ(board.byColor(Color::BLACK).b_and(board.byColor(Color::WHITE)), BitBoard::EMPTY);
			}
		}
	}

	return true;
}

template<> bool test<7>() {
	constexpr auto testName = "BoardTest(SEE)";

	const std::tuple<std::string, std::string, Value> SEE_TESTS[] = {
		{ "8/8/5R2/8/8/1kb5/8/2K5 b - - 0 1", "c3f6", scores::SIMPLIFIED_PIECE_VALUES[Piece::ROOK_WHITE] },
		{ "8/2k5/3b4/4n3/6N1/8/5K2/8 w - - 0 1", "g4e5", 0 },
		{ "k7/3q4/8/8/3Q4/4K3/8/8 b - - 0 1", "d7d4", 0 },
		{ "k7/3q4/4n3/8/3Q4/4K3/8/8 b - - 0 1", "d7d4", scores::SIMPLIFIED_PIECE_VALUES[Piece::QUEEN_WHITE] },
		{ "1k6/5n2/8/4p3/3P4/8/1B6/2K5 w - - 0 1", "d4e5", scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_BLACK] },
		{ "2r3k1/2r5/2r5/8/8/2R5/2R5/2R3K1 w - - 0 1", "c3c6", scores::SIMPLIFIED_PIECE_VALUES[Piece::ROOK_BLACK] },
		{ "6k1/7p/8/8/8/8/2Q5/6K1 w - - 0 1", "c2h7", 
			scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_BLACK] - scores::SIMPLIFIED_PIECE_VALUES[Piece::QUEEN_WHITE] },
		{ "8/3P4/8/8/8/k7/8/1K6 w - - 0 1", "d7d8r", 
			scores::SIMPLIFIED_PIECE_VALUES[Piece::ROOK_WHITE] - scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE] },
		{ "2n5/3P4/8/8/8/k7/8/1K6 w - - 0 1", "d7c8n", 
			scores::SIMPLIFIED_PIECE_VALUES[Piece::KNIGHT_WHITE] * 2 - scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE] },
		{ "rnbqkbnr/pp1ppppp/8/8/2pPP3/5P2/PPP3PP/RNBQKBNR b KQkq d3 0 1", "c4d3", 0 },
	};

	for (const auto& [fen, moveStr, expectedValue] : SEE_TESTS) {
		bool success;
		Board board = Board::fromFEN(fen, success);
		Move m = board.makeMoveFromString(moveStr);
		Value see = board.SEE(m);

		EXPECT_EQ(see, expectedValue);
	}

	return true;
}

template<> bool test<8>() {
	constexpr auto testName = "BoardTest(perftTest)";

	// Results of the perft function for depth 5
	const u64 PERFT_RESULTS[] = {
		4865609,
		193690690,
		674624,
		15833292,
		15833292,
		89941194,
		164075551
	};

	for (u32 i = 0; i < std::size(TEST_FENS); ++i) {
		bool success;
		Board board = Board::fromFEN(TEST_FENS[i], success);

		EXPECT_EQ(engine::perft(board, 5), PERFT_RESULTS[i]);
	}

	return true;
}


template<u32 Id>
void runTestsSequence() {
	using namespace std::chrono;

	if constexpr (Id) {
		runTestsSequence<Id - 1>();
	}

	auto start = high_resolution_clock::now();
	bool result = test<Id>();
	auto testTime = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();

	if (result) {
		io::g_out << io::Color::Green << "Test " << Id << " passed in " << testTime << " ns\n" << io::Color::White;
	} else {
		io::g_out << io::Color::Red << "Test " << Id << " failed\n" << io::Color::White;
	}
}

void runTests() {
	runTestsSequence<8>();
}