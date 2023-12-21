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

#include "Board.h"
#include <cstring>

#include "Utils/ConsoleColor.h"
#include "Utils/StringUtils.h"

Board::Board() noexcept
	: m_material { 0, 0 },
	m_score { Score(), Score() },
	m_moveCount(1),
	m_side(Color::WHITE) {
	for (auto square : Square::iter()) {
		m_board[square] = Piece::NONE;
	}

	for (auto piece : Piece::iter()) {
		m_pieces[piece] = BitBoard::EMPTY;
	}

	for (auto color : Color::iter()) {
		m_piecesByColor[color] = BitBoard::EMPTY;
	}

	m_states.reserve(64);
	StateInfo& firstState = m_states.emplace_back();
}

Board::Board(Board&& other) noexcept
	: m_states(std::move(other.m_states)),
	m_material { other.m_material[0], other.m_material[1] },
	m_score { other.m_score[0], other.m_score[1] },
	m_moveCount(other.moveCount()),
	m_side(other.side()) {
	for (auto square : Square::iter()) {
		m_board[square] = other[square];
	}

	for (auto piece : Piece::iter()) {
		m_pieces[piece] = other.byPiece(piece);
	}

	for (auto color : Color::iter()) {
		m_piecesByColor[color] = other.byColor(color);
	}
}

void Board::operator=(Board&& other) noexcept {
	if (&other == this) {
		return;
	}

	memcpy(this, &other, sizeof(Board));
	memset(&other, 0, sizeof(Board));
}

Board Board::makeInitialPosition() noexcept {
	bool _;
	return fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", _);
}

Board Board::fromFEN(std::string_view fen, bool& success) {
	Board result;

	// Pieces
	Square sq = Square::A8;
	u32 i = 0;
	for (; i < fen.size() && fen[i] != ' '; i++) {
		char ch = fen[i];

		if (str_utils::isDigit(ch)) {
			sq = Square(sq + (ch - '0'));
			continue;
		} else if (ch == '/') {
			sq = Square(File::A, Rank(sq.backward().getRank() - 1));
			continue;
		}

		Piece piece = Piece::fromFENChar(ch);
		if (piece == Piece::NONE) {
			success = false;
			return result;
		}

		result[sq] = piece;
		result.byPiece(piece).set(sq);
		result.byColor(piece.getColor()).set(sq);
		result.materialByColor(piece.getColor()) += Material::materialOf(piece.getType());
		result.scoreByColor(piece.getColor()) += scores::PST[piece][sq];
		result.hash() ^= zobrist::PIECE[piece][sq];

		sq = sq.forward();
	}

	if (++i >= fen.size()) {
		success = false;
		return result;
	}

	// Side to move
	result.side() = Color::fromFENChar(fen[i++]);
	result.hash() ^= zobrist::SIDE[result.side()];
	if (i >= fen.size() - 1) {
		return result;
	}

	// Castling rights
	if (fen[++i] != '-') {
		while (i < fen.size() && fen[i] != ' ') {
			u8 castleMask = Castle::bitMaskFromFENChar(fen[i++]);
			result.castleRight() |= castleMask;
		}
	} else {
		++i;
	}

	if (i >= fen.size() - 1) {
		return result;
	}

	// En passant square
	if (fen[++i] != '-') {
		if (i >= fen.size() - 1) {
			success = false;
			return result;
		}

		result.ep() = Square::fromChars(fen[i], fen[i + 1]);
		++i;
	}

	if (++i >= fen.size() - 1) {
		return result;
	}

	// Fifty rule
	result.fiftyRule() = str_utils::fromString<u8>(fen, ++i);

	if (++i >= fen.size()) {
		return result;
	}

	// Move count
	u32 fenMoveCount = str_utils::fromString<u32>(fen, i);
	result.moveCount() = (fenMoveCount ? (2 * (fenMoveCount - 1)) : 0) + result.side().getOpposite();

	// Internal intitializing
	result.initInternalState();

	success = true;
	return result;
}

std::string Board::toFEN() const noexcept {
	std::string result;
	result.reserve(72); // 72 is enough for almost any possible position, 87 would suffice for absolutely any position

	// Pieces
	u32 empties = 0;
	for (auto rank : Rank::reverseIter()) {
		for (auto file : File::iter()) {
			Square sq = Square(file, rank);
			Piece piece = m_board[sq];

			if (piece == Piece::NONE) {
				++empties;
				continue;
			}

			if (empties) {
				result += ('0' + empties);
				empties = 0;
			}

			result += piece.toChar();
		}

		if (empties) {
			result += ('0' + empties);
			empties = 0;
		}

		result += '/';
	}

	if (result.back() == '/') {
		result.pop_back();
	}

	// Side
	result += (m_side == Color::BLACK ? " b " : " w ");

	// Castle rights
	if (Castle::hasAnyCastleRight(state().castleRight)) {
		for (auto color : Color::reverseIter()) {
			for (auto castle : { Castle::KING_CASTLE, Castle::QUEEN_CASTLE }) {
				if (Castle::hasCastleRight(state().castleRight, castle, color)) {
					result += Castle::toFENChar(castle, color);
				}
			}
		}

		result += ' ';
	} else {
		result += "- ";
	}

	// En passant square
	if (state().ep == Square::NO_POS) {
		result += "- ";
	} else {
		result += state().ep.toString();
		result += ' ';
	}

	// Fifty rule
	result += std::to_string(state().fiftyRule);
	result += ' ';

	// Move count
	result += std::to_string(m_moveCount ? (m_moveCount - m_side.getOpposite()) / 2 + 1 : 1);

	return result;
}

Move Board::makeMoveFromString(std::string_view str) const noexcept {
	if (str.size() < 4 && str != "0-0") {
		return Move::makeNullMove(); // Corrupted move string
	}

	Square from = Square::NO_POS;
	Square to = Square::NO_POS;

	if (str == "0-0") {
		from = Square::makeRelativeSquare(m_side, Square::E1);
		to = Square::makeRelativeSquare(m_side, Square::G1);
	} else if (str == "0-0-0") {
		from = Square::makeRelativeSquare(m_side, Square::E1);
		to = Square::makeRelativeSquare(m_side, Square::C1);
	} else {
		from = Square::fromChars(str[0], str[1]);
		to = Square::fromChars(str[2], str[3]);
	}

	if (!byColor(m_side).test(from) || byColor(m_side).test(to) || from == to) {
		return Move::makeNullMove(); // Illegal move
	}

	MoveList moves;
	generateMoves(moves);
	for (Move m : moves) {
		if (m.getFrom() == from && m.getTo() == to) {
			if (!isLegal(m)) {
				return Move::makeNullMove();
			}
			else if (m.getMoveType() == MoveType::PROMOTION) {
				const PieceType promoted = str.size() > 4 ? Piece::fromFENChar(str[4]).getType() : PieceType::KNIGHT;
				return Move(from, to, MoveType::PROMOTION, promoted);
			}

			return m;
		}
	}

	return Move::makeNullMove();
}

bool Board::isLegal(const Move m) const noexcept {
	const Square from = m.getFrom();
	const Square to = m.getTo();

	switch (m.getMoveType()) {
	case MoveType::SIMPLE: {
		if (m_board[from].getType() == PieceType::KING) { // For king, the move is illegal if the destination is under attack
			return computeAttackersOf(m_side.getOpposite(), to, allPieces().b_xor(BitBoard::fromSquare(from))) == 0;
		}
	} [[fallthrough]];
	case MoveType::PROMOTION: {
	// For other pieces, the move is illegal if it puts own king under attack
		return checkBlockers(m_side).test(from) == 0
			|| BitBoard::areAligned(from, to, king(m_side));
	} case MoveType::ENPASSANT: {
	// Only check if own king gets attacked with this move
		const Square kingSq = king(m_side);
		const Square capturedSq = m_side == Color::WHITE
			? to.backward(8)
			: to.forward(8);

		const BitBoard occ = allPieces()
			.b_xor(BitBoard::fromSquare(from))
			.b_xor(BitBoard::fromSquare(capturedSq))
			.b_or(BitBoard::fromSquare(to));

		return BitBoard::attacksOf(PieceType::ROOK, kingSq, occ).b_and(rooksAndQueens(m_side.getOpposite())) == 0
			&& BitBoard::attacksOf(PieceType::BISHOP, kingSq, occ).b_and(bishopsAndQueens(m_side.getOpposite())) == 0;
	} case MoveType::CASTLE: {
	// Check if the castling path is under enemy attack
		const u8 step = to.getFile() == File::G ? -1 : 1;
		for (Square sq = to; sq != from; sq = sq.forward(step)) {
			if (computeAttackersOf(m_side.getOpposite(), sq)) {
				return false; // The square is under enemy attack
			}
		}

		return true;
	} default: return false;
	}
}

void Board::makeMove(const Move m) noexcept {
	return m_side == Color::BLACK
		? makeMove<Color::BLACK>(m)
		: makeMove<Color::WHITE>(m);
}

template<Color::Value Side>
void Board::makeMove(const Move m) noexcept {
	StateInfo& st = pushNextState();

	const Square from = m.getFrom();
	const Square to = m.getTo();
	const MoveType mt = m.getMoveType();
	const Piece piece = m_board[from];

	// Updating general state

	st.ep = Square::NO_POS;
	++m_moveCount;
	m_side = Color(Side).getOpposite();
	st.hash ^= zobrist::MOVE_KEY;

	// Changes by move type

	switch (mt) {
	case MoveType::SIMPLE: {
		if ((st.captured = movePieceWithCapture<Side>(piece, from, to)) != Piece::NONE) {
			st.hash ^= zobrist::PIECE[st.captured][to];
			st.fiftyRule = 0;
		} else if (piece == Piece(Side, PieceType::PAWN)) {
			st.fiftyRule = 0;
			if (Square::distance(from, to) == 2) { // Double pawn push
				st.ep = Side == Color::WHITE
					? from.forward(8)
					: from.backward(8);
			}
		}

		st.hash ^= zobrist::PIECE[piece][from] ^ zobrist::PIECE[piece][to];

		// Castling rights update
		st.castleRight &= Castle::getCastleChangeMask(from);
		st.castleRight &= Castle::getCastleChangeMask(to);
	} break;
	case MoveType::PROMOTION: {
		Piece promoted = Piece(Side, m.getPromotedPiece());
		if (i32(to) - from != (Side == Color::WHITE ? 8 : -8)) {
			if ((st.captured = promotePawnWithCapture<Side>(promoted, from, to)) != Piece::NONE) {
				st.hash ^= zobrist::PIECE[st.captured][to];
			}
		} else {
			promotePawn<Side, true>(promoted, from, to);
		}

		st.hash ^= zobrist::PIECE[Piece(Side, PieceType::PAWN)][from] ^ zobrist::PIECE[Piece(Side, m.getPromotedPiece())][to];
		st.fiftyRule = 0;

		// Castling rights update
		st.castleRight &= Castle::getCastleChangeMask(from);
		st.castleRight &= Castle::getCastleChangeMask(to);
	} break;
	case MoveType::ENPASSANT: {
		constexpr Piece OurPawn = Piece(Side, PieceType::PAWN);

		doEnpassant<Side, true>(from, to);

		st.fiftyRule = 0;
		st.hash ^= zobrist::PIECE[OurPawn][from] ^ zobrist::PIECE[OurPawn][to];
	} break;
	case MoveType::CASTLE: {
		constexpr Piece OurKing = Piece(Side, PieceType::KING);
		constexpr Piece OurRook = Piece(Side, PieceType::ROOK);

		st.castleRight &= Castle::getCastleChangeMask(from); // King square
		st.castleRight |= Castle::getBitMaskFor(Castle::CASTLE_DONE, Side);

		doCastling<Side, true>(from, to);
		st.hash ^= zobrist::PIECE[OurKing][from] ^ zobrist::PIECE[OurKing][to];

		if (to.getFile() == File::G) { // King side castling
			constexpr Square ROOK_FROM = Square::makeRelativeSquare(Side, Square::H1);
			constexpr Square ROOK_TO = Square::makeRelativeSquare(Side, Square::F1);

			st.hash ^= zobrist::PIECE[OurRook][ROOK_FROM] ^ zobrist::PIECE[OurRook][ROOK_TO];
		} else { // Queen side castling
			constexpr Square ROOK_FROM = Square::makeRelativeSquare(Side, Square::A1);
			constexpr Square ROOK_TO = Square::makeRelativeSquare(Side, Square::D1);

			st.hash ^= zobrist::PIECE[OurRook][ROOK_FROM] ^ zobrist::PIECE[OurRook][ROOK_TO];
		}
	} break;
	default: break;
	}

	updateInternalState();

	// Updating repetitions
	if (Depth ply = std::min<Depth>(st.fiftyRule, st.movesFromNull); ply >= 4) {
		Depth to = m_states.size() - ply;
		Hash current = st.hash;
		for (Depth i = m_states.size() - 5; i >= to; i -= 2) {
			if (m_states[i].hash == current) {
				st.lastRepetition = m_states.size() - i;
				break;
			}
		}
	}
}

void Board::unmakeMove(const Move m) noexcept {
	return m_side == Color::BLACK
		? unmakeMove<Color::WHITE>(m)
		: unmakeMove<Color::BLACK>(m);
}

template<Color::Value Side>
void Board::unmakeMove(const Move m) noexcept  {
	assert(m_states.size());

	constexpr Color OppositeSide = Color(Side).getOpposite();
	constexpr Piece OurPawn = Piece(Side, PieceType::PAWN);
	constexpr Piece OppositePawn = Piece(OppositeSide, PieceType::PAWN);
	constexpr Piece OurRook = Piece(Side, PieceType::ROOK);
	constexpr Piece OurKing = Piece(Side, PieceType::KING);

	const Piece captured = state().captured;
	m_states.pop_back();

	--m_moveCount;
	m_side = Side;

	const Square from = m.getFrom();
	const Square to = m.getTo();
	const MoveType mt = m.getMoveType();
	const Piece piece = m_board[to];

	switch (mt) {
		case MoveType::SIMPLE: unmovePieceWithCapture<Side>(piece, captured, from, to); break;
		case MoveType::PROMOTION: {
			Piece promoted = Piece(Side, m.getPromotedPiece());
			if (captured) {
				unpromotePawnWithCapture<Side>(promoted, captured, from, to);
			} else {
				promotePawn<Side, false>(promoted, from, to);
			}
		} break;
		case MoveType::ENPASSANT: doEnpassant<Side, false>(from, to); break;
		case MoveType::CASTLE: doCastling<Side, false>(from, to);  break;
	default: break;
	}
}

template<movegen::GenerationMode Mode>
void Board::generateMoves(MoveList& moves) const noexcept {
	if constexpr (Mode == movegen::QUIET_CHECKS) {
		high_assert(!isInCheck());

		return m_side == Color::WHITE
			? generateMoves<Color::WHITE, movegen::QUIET_CHECKS>(moves)
			: generateMoves<Color::BLACK, movegen::QUIET_CHECKS>(moves);
	}

	moves.clear();

	if constexpr (Mode != movegen::CHECK_EVASIONS) {
		if (isInCheck()) { // In check we only consider evasions
			return m_side == Color::WHITE
				? generateMoves<Color::WHITE, movegen::CHECK_EVASIONS>(moves)
				: generateMoves<Color::BLACK, movegen::CHECK_EVASIONS>(moves);
		}
	}

	return m_side == Color::WHITE
		? generateMoves<Color::WHITE, Mode>(moves)
		: generateMoves<Color::BLACK, Mode>(moves);
}

// Instantiation (removed to the cpp file to boost compilation)
template void Board::generateMoves<movegen::ALL_MOVES>(MoveList& moves) const noexcept;
template void Board::generateMoves<movegen::CAPTURES>(MoveList& moves) const noexcept;
template void Board::generateMoves<movegen::CHECK_EVASIONS>(MoveList& moves) const noexcept;
template void Board::generateMoves<movegen::QUIET_CHECKS>(MoveList& moves) const noexcept;

template<Color::Value Side, movegen::GenerationMode Mode>
void Board::generateMoves(MoveList& moves) const noexcept {
	constexpr Color OpponentSide = Color(Side).getOpposite();
	constexpr Direction Up = Direction::makeRelativeDirection(Side, Direction::UP);
	constexpr Direction UpRight = Direction::makeRelativeDirection(Side, Direction::UPRIGHT);
	constexpr Direction UpLeft = Direction::makeRelativeDirection(Side, Direction::UPLEFT);
	constexpr Direction Down = Direction::makeRelativeDirection(Side, Direction::DOWN);
	constexpr Direction DownRight = Direction::makeRelativeDirection(Side, Direction::DOWNRIGHT);
	constexpr Direction DownLeft = Direction::makeRelativeDirection(Side, Direction::DOWNLEFT);
	constexpr Direction Right = Direction::makeRelativeDirection(Side, Direction::RIGHT);
	constexpr Direction Left = Direction::makeRelativeDirection(Side, Direction::LEFT);
	constexpr BitBoard Rank2BB = BitBoard::fromRank(Rank::makeRelativeRank(Side, Rank::R2));
	constexpr BitBoard Rank3BB = BitBoard::fromRank(Rank::makeRelativeRank(Side, Rank::R3));
	constexpr BitBoard Rank7BB = BitBoard::fromRank(Rank::makeRelativeRank(Side, Rank::R7));


	const BitBoard friendlyPieces = m_piecesByColor[Side];
	const BitBoard enemyPieces = Mode == movegen::CHECK_EVASIONS
		? checkGivers() // In check we can only capture the piece that gives the check
		: m_piecesByColor[OpponentSide];

	const BitBoard allPieces = this->allPieces();
	const BitBoard emptySquares = allPieces.b_not();
	const Square kingSq = king(Side);
	const Square opponentKingSq = king(OpponentSide);

	const BitBoard trg =
		Mode == movegen::CAPTURES
			? enemyPieces // For captures mode, we look only for moves where the to square has an enemy piece on it
		: Mode == movegen::CHECK_EVASIONS
			? BitBoard::betweenBits(kingSq, checkGivers().lsb()) // For check evasions we look only for moves that block the check
		: Mode == movegen::QUIET_CHECKS
			? allPieces.b_not() // In quiet checks we consider all targets but pieces
			: friendlyPieces.b_not(); // All the suitable targets in all moves mode


	// King

	BitBoard bb = byPiece(Piece(Side, PieceType::KING));
	if (Mode != movegen::QUIET_CHECKS || checkBlockers(OpponentSide).test(kingSq)) {
		BitBoard attacks = BitBoard::attacksOf(PieceType::KING, kingSq, allPieces)
			.b_and(Mode != movegen::CHECK_EVASIONS ? trg : friendlyPieces.b_not());

		if constexpr (Mode == movegen::QUIET_CHECKS) {
			attacks = attacks.b_and(BitBoard::pseudoAttacks<PieceType::QUEEN>(opponentKingSq).b_not());
		}

		BB_FOR_EACH(sq, attacks) {
			moves.emplace(kingSq, sq);
		}

		if constexpr (Mode == movegen::CHECK_EVASIONS) {
			if (checkGivers().hasMoreThanOne()) { // Double check
				return; // No moves but king's can evade double check
			}
		}
	}

	// Pawns

	bb = m_pieces[Piece(Side, PieceType::PAWN)];

	const BitBoard promotablePawns = bb.b_and(Rank7BB);
	const BitBoard nonPromotablePawns = bb.b_xor(promotablePawns);

	// Pawn promotions
	if (Mode != movegen::QUIET_CHECKS && promotablePawns) {
		BitBoard upPromotions = promotablePawns.shift(Up).b_and(emptySquares);
		BitBoard upLeftPromotions = promotablePawns.shift(UpLeft).b_and(enemyPieces);
		BitBoard upRightPromotions = promotablePawns.shift(UpRight).b_and(enemyPieces);

		if constexpr (Mode == movegen::CHECK_EVASIONS) {
			// In check we consider only moves that can block the check
			upPromotions = upPromotions.b_and(trg);
		}

		BB_FOR_EACH(sq, upPromotions) {
			moves.emplace<MoveType::PROMOTION>(sq.shift(Down), sq, PieceType::QUEEN);
			if constexpr (Mode != movegen::CAPTURES) {
				moves.emplace<MoveType::PROMOTION>(sq.shift(Down), sq, PieceType::ROOK);
				moves.emplace<MoveType::PROMOTION>(sq.shift(Down), sq, PieceType::BISHOP);
				moves.emplace<MoveType::PROMOTION>(sq.shift(Down), sq, PieceType::KNIGHT);
			}
		}

		BB_FOR_EACH(sq, upLeftPromotions) {
			moves.emplace<MoveType::PROMOTION>(sq.shift(DownRight), sq, PieceType::QUEEN);
			if constexpr (Mode != movegen::CAPTURES) {
				moves.emplace<MoveType::PROMOTION>(sq.shift(DownRight), sq, PieceType::ROOK);
				moves.emplace<MoveType::PROMOTION>(sq.shift(DownRight), sq, PieceType::BISHOP);
				moves.emplace<MoveType::PROMOTION>(sq.shift(DownRight), sq, PieceType::KNIGHT);
			}
		}

		BB_FOR_EACH(sq, upRightPromotions) {
			moves.emplace<MoveType::PROMOTION>(sq.shift(DownLeft), sq, PieceType::QUEEN);
			if constexpr (Mode != movegen::CAPTURES) {
				moves.emplace<MoveType::PROMOTION>(sq.shift(DownLeft), sq, PieceType::ROOK);
				moves.emplace<MoveType::PROMOTION>(sq.shift(DownLeft), sq, PieceType::BISHOP);
				moves.emplace<MoveType::PROMOTION>(sq.shift(DownLeft), sq, PieceType::KNIGHT);
			}
		}
	}

	// Pawn captures
	if (Mode != movegen::QUIET_CHECKS && nonPromotablePawns) {
		BitBoard upLeftCaptures = nonPromotablePawns.shift(UpLeft).b_and(enemyPieces);
		BitBoard upRightCaptures = nonPromotablePawns.shift(UpRight).b_and(enemyPieces);

		BB_FOR_EACH(sq, upLeftCaptures) {
			moves.emplace(sq.shift(DownRight), sq);
		}

		BB_FOR_EACH(sq, upRightCaptures) {
			moves.emplace(sq.shift(DownLeft), sq);
		}

		// En passant capture
		if (state().ep != Square::NO_POS) {
			BitBoard epCapture = bb.b_and(BitBoard::fromSquare(state().ep).pawnAttackedSquares<OpponentSide.value()>());
			while (epCapture) {
				moves.emplace<MoveType::ENPASSANT>(epCapture.pop(), state().ep);
			}
		}
	}

	// Quiet pawn moves
	if constexpr (Mode != movegen::CAPTURES) {
		BitBoard singlePawnPush = nonPromotablePawns.shift(Up).b_and(emptySquares);
		BitBoard doublePawnPush = singlePawnPush.b_and(Rank3BB).shift(Up).b_and(emptySquares);

		if constexpr (Mode == movegen::CHECK_EVASIONS) {
			// In check we consider only moves that can block the check
			singlePawnPush = singlePawnPush.b_and(trg);
			doublePawnPush = doublePawnPush.b_and(trg);
		} else if constexpr (Mode == movegen::QUIET_CHECKS) {
			const BitBoard pawnToKingAttacks = BitBoard::pawnAttacks(OpponentSide, opponentKingSq);
			BitBoard pawnsBlockingCheck = checkBlockers(OpponentSide).b_and(BitBoard::fromFile(opponentKingSq.getFile()).b_not());

			singlePawnPush = singlePawnPush.b_and(pawnToKingAttacks.b_or(pawnsBlockingCheck = pawnsBlockingCheck.shift(Up)));
			doublePawnPush = doublePawnPush.b_and(pawnToKingAttacks.b_or(pawnsBlockingCheck.shift(Up)));
		}

		BB_FOR_EACH(sq, singlePawnPush) {
			moves.emplace(sq.shift(Down), sq);
		}

		BB_FOR_EACH(sq, doublePawnPush) {
			moves.emplace(sq.shift(Down).shift(Down), sq);
		}
	}

	// Pieces: knight, bishop, rook, queen
	generatePieceMoves<Side, Mode, PieceType::KNIGHT>(moves, allPieces, trg);
	generatePieceMoves<Side, Mode, PieceType::BISHOP>(moves, allPieces, trg);
	generatePieceMoves<Side, Mode, PieceType::ROOK>(moves, allPieces, trg);
	generatePieceMoves<Side, Mode, PieceType::QUEEN>(moves, allPieces, trg);

	// Castlings
	if constexpr (Mode == movegen::ALL_MOVES) {
		if (Castle::hasCastleRight(state().castleRight, Castle::KING_CASTLE, Side)
			&& (BitBoard::castlingInternalSquares(Side, Castle::KING_CASTLE) & allPieces) == 0) {
			moves.emplace<MoveType::CASTLE>(kingSq, Square(File::G, Rank::makeRelativeRank(Side, Rank::R1)));
		}

		if (Castle::hasCastleRight(state().castleRight, Castle::QUEEN_CASTLE, Side)
			&& (BitBoard::castlingInternalSquares(Side, Castle::QUEEN_CASTLE) & allPieces) == 0) {
			moves.emplace<MoveType::CASTLE>(kingSq, Square(File::C, Rank::makeRelativeRank(Side, Rank::R1)));
		}
	}
}

std::ostream& operator<<(std::ostream& out, const Board& board) noexcept {
	out << "-----------------------------------\n";
	for (auto rank : Rank::reverseIter()) {
		for (auto file : File::iter()) {
			Piece piece = board[Square(file, rank)];
			io::Color color = piece == Piece::NONE
				? io::Color::White
			: piece.getColor() == Color::WHITE
				? io::Color::Green
				: io::Color::Magenta;
			out << " | " << color << piece.toChar() << io::Color::White;
		}

		out << " |\n-----------------------------------\n";
	}

	out << "FEN: " << board.toFEN() << std::endl;

	return out;
}

Value Board::SEE(const Move m) const noexcept {
	const Square to = m.getTo();
	Square from = m.getFrom();
	BitBoard occ = allPieces();
	Value result; // Current score
	Value nextLoss; // The next value to be lost in a capture

	// Preparations
	switch (m.getMoveType()) {
		case MoveType::PROMOTION: {
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece(Color::WHITE, m.getPromotedPiece())];
			result = scores::SIMPLIFIED_PIECE_VALUES[m_board[to]]
				+ nextLoss - scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE];
			occ.clear(from);
		} break;
		case MoveType::SIMPLE: {
			result = scores::SIMPLIFIED_PIECE_VALUES[m_board[to]];
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[m_board[from]];
			occ.clear(from);
		} break;
		case MoveType::ENPASSANT: {
			const Square capturedSq = Square(to.getFile(), from.getRank());
			result = scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE];
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE];
			occ.clear(capturedSq);
			occ.clear(from);
		} break;
		case MoveType::CASTLE: return 0; // Do not consider castlings
	default: return 0;
	}

	// The actual SEE algorithm starts here
	Value valuesArr[36] = { result };
	i32 i = 0;

	Color side = m_side;
	BitBoard attackers = computeAllAttackersOf(to, occ);
	BitBoard currentAttackers;
	i8 modifier = 1;

	while (true) {
		side = side.getOpposite();
		attackers = attackers.b_and(occ);
		currentAttackers = attackers.b_and(byColor(side));

		if (occ.b_and(state().pinners[side.getOpposite()])) {
			currentAttackers = currentAttackers.b_and(checkBlockers(side).b_not());
		}

		if (!currentAttackers) {
			break;
		}

		modifier = -modifier;

		// Looking for the least valuable attacker

		// Pawns
		BitBoard b = currentAttackers.b_and(byPiece(Piece(side, PieceType::PAWN)));
		if (b) { // Capture with a pawn is never a loss
			result += modifier * nextLoss;
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece::PAWN_WHITE];
			valuesArr[++i] = result;

			// Updating the attackers
			occ.clear(b.lsb());
			attackers = attackers.b_or(BitBoard::attacksOf(PieceType::BISHOP, to, occ)
									   .b_and(bishopsAndQueens(Color::WHITE).b_or(bishopsAndQueens(Color::BLACK))));

			continue;
		}

		// Knights
		b = currentAttackers.b_and(byPiece(Piece(side, PieceType::KNIGHT)));
		if (b) {
			result += modifier * nextLoss;
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece::KNIGHT_WHITE];
			valuesArr[++i] = result;

			occ.clear(b.lsb());
			continue;
		}

		// Bishops
		b = currentAttackers.b_and(bishops(side));
		if (b) {
			result += modifier * nextLoss;
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece::BISHOP_WHITE];
			valuesArr[++i] = result;

			// Updating the attackers
			occ.clear(b.lsb());
			attackers = attackers.b_or(BitBoard::attacksOf(PieceType::BISHOP, to, occ)
									   .b_and(bishopsAndQueens(Color::WHITE).b_or(bishopsAndQueens(Color::BLACK))));
			continue;
		}

		// Rooks
		b = currentAttackers.b_and(rooks(side));
		if (b) {
			result += modifier * nextLoss;
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece::ROOK_WHITE];
			valuesArr[++i] = result;

			// Updating the attackers
			occ.clear(b.lsb());
			attackers = attackers.b_or(BitBoard::attacksOf(PieceType::ROOK, to, occ)
									   .b_and(rooksAndQueens(Color::WHITE).b_or(rooksAndQueens(Color::BLACK))));
			continue;
		}

		// Queens
		b = currentAttackers.b_and(queens(side));
		if (b) {
			result += modifier * nextLoss;
			nextLoss = scores::SIMPLIFIED_PIECE_VALUES[Piece::QUEEN_WHITE];
			valuesArr[++i] = result;

			// Updating the attackers
			occ.clear(b.lsb());
			attackers = attackers.b_or(BitBoard::attacksOf(PieceType::BISHOP, to, occ)
									   .b_and(bishopsAndQueens(Color::WHITE).b_or(bishopsAndQueens(Color::BLACK))))
				.b_or(BitBoard::attacksOf(PieceType::ROOK, to, occ)
					  .b_and(rooksAndQueens(Color::WHITE).b_or(rooksAndQueens(Color::BLACK))));
			continue;
		}

		// King
		// This the last we can make a capture with, so no need to continue the loop
		// We can do this capture only if there are no attackers from the other side
		if (attackers.b_and(byColor(side.getOpposite())).b_and(occ) == BitBoard::EMPTY) {
			b = currentAttackers.b_and(byPiece(Piece(side, PieceType::KING)));
			if (b) {
				result += modifier * nextLoss;
			}

			valuesArr[++i] = result;
		}

		break;
	}

	for (; i > 0; --i) {
		if (i & 1) {
			valuesArr[i - 1] = std::min(valuesArr[i - 1], valuesArr[i]);
		} else {
			valuesArr[i - 1] = std::max(valuesArr[i - 1], valuesArr[i]);
		}
	}

	return valuesArr[0];
}
