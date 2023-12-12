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
#include <vector>

#include "Defs.h"
#include "BitBoard.h"
#include "MoveGenerationUtils.h"
#include "Score.h"
#include "Zobrist.h"

/*
*	Board(.h/.cpp) contains the class that handles the state of chessboard
*	and game overall.
* 
*	To create an initial position, Forsyth-Edwards Notation (FEN) is used.
*/

class Board final {
private:
	// StateInfo contains some information that allows to undo a move and stores the information about the board state
	struct StateInfo final {
		BitBoard checkBlockers[Color::VALUES_COUNT] { BitBoard::EMPTY, BitBoard::EMPTY };
		BitBoard pinners[Color::VALUES_COUNT] { BitBoard::EMPTY, BitBoard::EMPTY };
		BitBoard checkGivers = BitBoard::EMPTY;
		Hash hash = 0;

		// Contains how much moves ago was the last repetition of the position
		// 0 dy default - which means no repetitions of the position occured yet
		Depth lastRepetition = 0;

		u32 movesFromNull = 0; // Number of moves since the last null move

		Square ep = Square::NO_POS;
		Piece captured = Piece::NONE;
		u8 fiftyRule = 0;
		u8 castleRight = 0;
	};

private:
	// Pieces info
	Piece m_board[64];
	BitBoard m_pieces[Piece::VALUES_COUNT];
	BitBoard m_piecesByColor[Color::VALUES_COUNT];

	// Info not related to the board by itself
	std::vector<StateInfo> m_states;

	i32 m_material[Color::VALUES_COUNT];
	Score m_score[Color::VALUES_COUNT]; // Scores according to scores::PST
	u32 m_moveCount;

	// Game state info
	Color m_side; // The side to do a move

public:

	///  CONSTRUCTORS  ///

	Board() noexcept;
	Board(Board&& other) noexcept;

	void operator=(Board&& other) noexcept;


	///  FEN  ///

	static Board makeInitialPosition() noexcept;

	// Creates a board from the Forsyth-Edwards Notation
	static Board fromFEN(std::string_view fen, bool& success);
	std::string toFEN() const noexcept;


	///  AUXILIARY OUTER METHODS  ///

	// Returns null move if the move is illegal
	Move makeMoveFromString(std::string_view str) const noexcept;


	///  OPERATORS  ///

	CM_PURE constexpr Piece& operator[](const Square sq) noexcept {
		high_assert(sq != Square::NO_POS);

		return m_board[sq];
	};

	CM_PURE constexpr Piece operator[](const Square sq) const noexcept {
		high_assert(sq != Square::NO_POS);

		return m_board[sq];
	};

	friend std::ostream& operator<<(std::ostream& out, const Board& board) noexcept;


	///  MOVE RELATED METHODS  ///

	// Checks if a pseudo-legal move is legal
	bool isLegal(const Move m) const noexcept;
	void makeMove(const Move m) noexcept;

	template<Color::Value Side>
	void makeMove(const Move m) noexcept;

	void unmakeMove(const Move m) noexcept;

	template<Color::Value Side>
	void unmakeMove(const Move m) noexcept;

	// Null move is just skipping side's move
	// It is not legal in the actual game
	// Only used in the search engine
	INLINE void makeNullMove() noexcept {
		m_side = m_side.getOpposite();
		StateInfo& st = pushNextState();
		st.hash ^= zobrist::NULL_MOVE_KEY;
		st.movesFromNull = 0;

		updateInternalState();
	}

	INLINE void unmakeNullMove() noexcept {
		assert(m_states.size());

		m_side = m_side.getOpposite();
		m_states.pop_back();
	}

	template<movegen::GenerationMode Mode = movegen::ALL_MOVES>
	void generateMoves(MoveList& moves) const noexcept;

	template<Color::Value Side, movegen::GenerationMode Mode>
	void generateMoves(MoveList& moves) const noexcept;


	template<Color::Value Side, movegen::GenerationMode Mode, PieceType::Value PT>
	INLINE constexpr void generatePieceMoves(MoveList& moves, const BitBoard allPieces, const BitBoard trg) const noexcept {
		static_assert(PT != PieceType::NONE && PT != PieceType::PAWN && PT != PieceType::KING);

		BitBoard pieces = byPiece(Piece(Side, PT));
		BB_FOR_EACH(sq, pieces) {
			BitBoard attacks = BitBoard::attacksOf(PT, sq, allPieces).b_and(trg);
			while (attacks) {
				moves.emplace(sq, attacks.pop());
			}
		}
	}


	///  GAME RESULT RELATED METHODS  ///

	// Checks if there is not enough material
	// If both sides have low material - it is a draw
	CM_PURE bool lowMaterialDraw() const noexcept {
		if (byPieceType(PieceType::PAWN) != BitBoard::EMPTY) {
			return false; // It cannot be a draw if there are any pawns left
		} else if (m_material[Color::WHITE] < 5 && m_material[Color::BLACK] < 5) {
			return true; // Since there are no pawns, the sides at most have a bishop or a knight each - not enough
		}

		return false;
	}

	// If there are 50 moves since the last irreversible move - it is a draw
	// Irreversible moves are: captures, castlings, pawn pushes and promotions
	CM_PURE bool fiftyRuleDraw() const noexcept {
		return state().fiftyRule >= 100;
	}

	// If some position was repeated 3 times - it is a draw
	// While in search, for simplification, we consider repetition 
	// for 2 times as a draw
	// Ply is the search ply, 0 by default makes it look for triple repetition
	CM_PURE bool repetitionDraw(const Depth ply = 0) const noexcept {
		if (Depth lastRep = state().lastRepetition; lastRep) {
			return lastRep <= ply
				? true // True if the position repeated itself during the search
				: m_states[m_states.size() - lastRep].lastRepetition != 0;
		}

		return false;
	}

	CM_PURE bool isDraw(const Depth ply = 0) const noexcept {
		return lowMaterialDraw() || fiftyRuleDraw() || repetitionDraw(ply);
	}

	// Checks if the game has reached an end
	// Returns NONE if there is no result yet
	// Note: this function is not supposed to be used in search
	// It is slow, since it uses movegen and checks all the moves
	CM_PURE GameResult computeGameResult() const noexcept {
		if (isDraw()) {
			return GameResult::DRAW;
		}

		MoveList ml;
		generateMoves(ml);
		for (Move m : ml) {
			if (isLegal(m)) {
				return GameResult::NONE; // There is a legal move
			}
		}

		// If the side has no legal moves, it is a game end
		return isInCheck() 
			? (m_side == Color::WHITE ? GameResult::BLACK_WON : GameResult::WHITE_WON) // Mate
			: GameResult::DRAW; // Stalemate
	}


	///  INTERNAL STATE UPDATE  ///

	// Setups the board once it was loaded
	INLINE void initInternalState() noexcept {
		state().checkGivers = computeAttackersOf(m_side.getOpposite(), king(m_side));

		updateInternalState();
	}

	INLINE void updateInternalState() noexcept {
		state().checkGivers = computeAttackersOf(m_side.getOpposite(), king(m_side));

		updateInternalState(Color::WHITE);
		updateInternalState(Color::BLACK);
	}

	// Updates the internal state for the given side
	INLINE void updateInternalState(const Color side) noexcept {
		const Square kingSq = king(side);
		StateInfo& st = state();
		st.checkBlockers[side] = 0;

		BitBoard snipers = BitBoard::pseudoAttacks<PieceType::BISHOP>(kingSq).b_and(bishopsAndQueens(side.getOpposite()))
						  .b_or(BitBoard::pseudoAttacks<PieceType::ROOK>(kingSq).b_and(rooksAndQueens(side.getOpposite())));
		BitBoard occupancy = allPieces() ^ snipers;

		BB_FOR_EACH(sq, snipers) {
			BitBoard b = BitBoard::betweenBits(kingSq, sq).b_and(occupancy);

			if (b && !b.hasMoreThanOne()) {
				st.checkBlockers[side] |= b;

				if (b.b_and(byColor(side))) {
					st.pinners[side.getOpposite()].set(sq);
				}
			}
		}
	}

	// Creates and pushes a new state copying some of the previous one and updating some trivial fields
	INLINE StateInfo& pushNextState() noexcept {
		StateInfo& result = m_states.emplace_back();
		StateInfo& prev = m_states[m_states.size() - 2];

		result.castleRight = prev.castleRight;
		result.fiftyRule = prev.fiftyRule + 1;
		result.movesFromNull = prev.movesFromNull + 1;
		result.hash = prev.hash;

		return result;
	}


	///  CM_PURE METHODS  ///

	CM_PURE constexpr BitBoard computeAttackersOf(const Color side, const Square sq) const noexcept {
		return computeAttackersOf(side, sq, allPieces());
	}

	CM_PURE constexpr BitBoard computeAttackersOf(const Color side, const Square sq, const BitBoard occ) const noexcept {
		return side == Color::WHITE
			? computeAttackersOf<Color::WHITE>(sq, occ)
			: computeAttackersOf<Color::BLACK>(sq, occ);
	}

	// Side is the color of the attacking side
	template<Color::Value Side>
	CM_PURE constexpr BitBoard computeAttackersOf(const Square sq, const BitBoard occ) const noexcept {
		constexpr Color::Value OppositeSide = Color(Side).getOpposite().value();

		return BitBoard::pawnAttacks(OppositeSide, sq).b_and(byPiece(Piece(Side, PieceType::PAWN)))
			.b_or(BitBoard::attacksOf(PieceType::KNIGHT, sq, occ).b_and(byPiece(Piece(Side, PieceType::KNIGHT))))
			.b_or(BitBoard::attacksOf(PieceType::BISHOP, sq, occ).b_and(byPiece(Piece(Side, PieceType::BISHOP))))
			.b_or(BitBoard::attacksOf(PieceType::ROOK,   sq, occ).b_and(byPiece(Piece(Side, PieceType::ROOK))))
			.b_or(BitBoard::attacksOf(PieceType::QUEEN,  sq, occ).b_and(byPiece(Piece(Side, PieceType::QUEEN))))
			.b_or(BitBoard::attacksOf(PieceType::KING,   sq, occ).b_and(byPiece(Piece(Side, PieceType::KING))));
	}

	// Finds attackers from both sides
	CM_PURE constexpr BitBoard computeAllAttackersOf(const Square sq, const BitBoard occ) const noexcept {
		return computeAttackersOf<Color::BLACK>(sq, occ).b_or(computeAttackersOf<Color::WHITE>(sq, occ));
	}

	CM_PURE constexpr BitBoard allPieces() const noexcept {
		return m_piecesByColor[Color::WHITE].b_or(m_piecesByColor[Color::BLACK]);
	}

	CM_PURE constexpr BitBoard byPieceType(const PieceType pt) const noexcept {
		return m_pieces[Piece(Color::WHITE, pt)].b_or(m_pieces[Piece(Color::BLACK, pt)]);
	}

	CM_PURE constexpr BitBoard bishops(const Color color) const noexcept {
		return m_pieces[Piece(color, PieceType::BISHOP)];
	}

	CM_PURE constexpr BitBoard rooks(const Color color) const noexcept {
		return m_pieces[Piece(color, PieceType::ROOK)];
	}

	CM_PURE constexpr BitBoard queens(const Color color) const noexcept {
		return m_pieces[Piece(color, PieceType::QUEEN)];
	}

	CM_PURE constexpr BitBoard rooksAndQueens(const Color color) const noexcept {
		return rooks(color).b_or(queens(color));
	}

	CM_PURE constexpr BitBoard bishopsAndQueens(const Color color) const noexcept {
		return bishops(color).b_or(queens(color));
	}

	// Returns the king's square
	CM_PURE constexpr Square king(const Color color) const noexcept {
		return m_pieces[Piece(color, PieceType::KING)].lsb();
	}

	CM_PURE bool isInCheck() const noexcept {
		return state().checkGivers != BitBoard::EMPTY;
	}


	///  GETTERS  ///

	CM_PURE StateInfo& state() noexcept {
		return m_states.back();
	}

	CM_PURE const StateInfo& state() const noexcept {
		return m_states.back();
	}

	CM_PURE BitBoard checkBlockers(const Color side) const noexcept {
		return state().checkBlockers[side];
	}

	CM_PURE BitBoard checkGivers() const noexcept {
		return state().checkGivers;
	}

	// Returns the bitboard for the given piece
	CM_PURE constexpr BitBoard& byPiece(const Piece pt) noexcept {
		return m_pieces[pt];
	}

	// Returns the bitboard for the given piece
	CM_PURE constexpr BitBoard byPiece(const Piece pt) const noexcept {
		return m_pieces[pt];
	}

	// Returns the bitboard for the given color
	CM_PURE constexpr BitBoard& byColor(const Color color) noexcept {
		return m_piecesByColor[color];
	}

	// Returns the bitboard for the given color
	CM_PURE constexpr BitBoard byColor(const Color color) const noexcept {
		return m_piecesByColor[color];
	}

	CM_PURE constexpr Score& scoreByColor(const Color color) noexcept {
		return m_score[color];
	}

	CM_PURE constexpr Score scoreByColor(const Color color) const noexcept {
		return m_score[color];
	}

	// Returns the score for white
	CM_PURE constexpr Score score() const noexcept {
		return m_score[Color::WHITE] - m_score[Color::BLACK];
	}

	CM_PURE constexpr i32& materialByColor(const Color color) noexcept {
		return m_material[color];
	}

	CM_PURE constexpr i32 materialByColor(const Color color) const noexcept {
		return m_material[color];
	}

	CM_PURE Hash computeHash() const noexcept {
		return hash()
			^ zobrist::SIDE[m_side]
			^ (state().ep != Square::NO_POS ? zobrist::EP[state().ep.getFile()] : 0ull)
			^ zobrist::CASTLING[state().castleRight];
	}

	// The position hash key
	CM_PURE Hash& hash() noexcept {
		return state().hash;
	}

	// The position hash key
	CM_PURE Hash hash() const noexcept {
		return state().hash;
	}

	// The number of moves made from the initial position
	CM_PURE constexpr u32& moveCount() noexcept {
		return m_moveCount;
	}

	// The number of moves made from the initial position
	CM_PURE constexpr u32 moveCount() const noexcept {
		return m_moveCount;
	}

	// Side to do move
	CM_PURE constexpr Color& side() noexcept {
		return m_side;
	}

	// Side to do move
	CM_PURE constexpr Color side() const noexcept {
		return m_side;
	}

	// En passant square
	CM_PURE Square& ep() noexcept {
		return state().ep;
	}

	// En passant square
	CM_PURE Square ep() const noexcept {
		return state().ep;
	}

	// Returns the number of moves counted by the fifty rule (moves since last capture/pawn advance)
	CM_PURE u8& fiftyRule() noexcept {
		return state().fiftyRule;
	}

	// Returns the number of moves counted by the fifty rule (moves since last capture/pawn advance)
	CM_PURE u8 fiftyRule() const noexcept {
		return state().fiftyRule;
	}

	CM_PURE u8& castleRight() noexcept {
		return state().castleRight;
	}

	CM_PURE u8 castleRight() const noexcept {
		return state().castleRight;
	}
};