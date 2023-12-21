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

#include "PawnHashTable.h"
#include "Scores.h"

namespace engine {
    PawnHashEntry PawnHashTable::s_table[1 << PAWN_HASH_TABLE_SIZE_LOG2];

    void PawnHashTable::init() {
        memset(s_table, 0, sizeof(s_table));
    }

    PawnHashEntry& PawnHashTable::getOrScanPHE(Board& board) {
        const BitBoard wpawns = board.byPiece(Piece::PAWN_WHITE);
        const BitBoard bpawns = board.byPiece(Piece::PAWN_BLACK);

        Hash hash = (wpawns ^ bpawns) >> 8; // Only 48 bits matter
        hash = (hash ^ (hash >> PAWN_HASH_TABLE_SIZE_LOG2) ^ (hash >> (PAWN_HASH_TABLE_SIZE_LOG2 * 2)) ^ (hash >> (PAWN_HASH_TABLE_SIZE_LOG2 * 3)));
        hash &= (1 << PAWN_HASH_TABLE_SIZE_LOG2) - 1;

        PawnHashEntry& entry = s_table[hash];
        if (entry.pawns[Color::WHITE] == wpawns && entry.pawns[Color::BLACK] == bpawns) {
            return entry;
        }
        
        // Scanning the pawns information from the board
		memset(&entry, 0, sizeof(PawnHashEntry));

		entry.pawns[Color::WHITE] = wpawns;
		entry.pawns[Color::BLACK] = bpawns;

		scanPawns<Color::WHITE>(board, entry);
		scanPawns<Color::BLACK>(board, entry);

		return entry;
    }

	template<Color::Value Side>
	void PawnHashTable::scanPawns(Board& board, PawnHashEntry& entry) {
		constexpr Color::Value OppositeSide = Color(Side).getOpposite().value();
		constexpr Direction::Value Up = Direction::makeRelativeDirection(Side, Direction::UP).value();
		constexpr Direction::Value Down = Direction::makeRelativeDirection(Side, Direction::DOWN).value();
		constexpr Rank Rank2 = Rank::makeRelativeRank(Side, Rank::R2);
		constexpr BitBoard BBRank2 = BitBoard::fromRank(Rank2);

		const BitBoard pawns = board.byPiece(Piece(Side, PieceType::PAWN));
		const BitBoard enemyPawns = board.byPiece(Piece(OppositeSide, PieceType::PAWN));

		const BitBoard ourPawnAttacks = pawns.pawnAttackedSquares<Side>();

		BitBoard pieces = pawns;
		BB_FOR_EACH(sq, pieces) {
			// Most advanced ranks
			entry.mostAdvanced[Side][sq.getFile() + 1] = std::max(entry.mostAdvanced[Side][sq.getFile() + 1], Rank::makeRelativeRank(Side, sq.getRank()));

			// Counting islands
			if (File f = sq.getFile(); f == File::H || BitBoard::fromFile(File::Value(f + 1)).b_and(pawns) == BitBoard::EMPTY) {
				entry.islandsCount[Side]++;
			} else if (BitBoard pawnsOnNextFile = BitBoard::fromFile(File::Value(f + 1)).b_and(pawns); pawnsOnNextFile != BitBoard::EMPTY) {
				entry.distortion[Side] += std::max(0, std::abs(pawnsOnNextFile.lsb().getRank() - sq.getRank()) - 1);
			}

			// Defended pawns
			if (ourPawnAttacks.test(sq)) {
				entry.pawnEvaluation[Side] += scores::DEFENDED_PAWN[Rank::makeRelativeRank(Side, sq.getRank())];
			}

			if (BitBoard::threeFilesForward<Side>(sq).b_and(enemyPawns) == BitBoard::EMPTY
				&& BitBoard::directionBits<Up>(sq).b_and(pawns) == BitBoard::EMPTY) { // Passed pawn
				entry.pawnEvaluation[Side] += scores::PASSED_PAWN[Rank::makeRelativeRank(Side, sq.getRank())];
				entry.passed.set(sq);
			}

			// Isolated pawn
			if (BitBoard::adjacentFiles(sq.getFile()).b_and(pawns) == BitBoard::EMPTY) {
				entry.pawnEvaluation[Side] += scores::ISOLATED_PAWN;
				entry.isolated.set(sq);
			}

			// Double pawn
			if (BitBoard::directionBits<Up>(sq).b_and(pawns) != BitBoard::EMPTY) {
				entry.pawnEvaluation[Side] += scores::DOUBLE_PAWN;
				entry.doubled.set(sq);
			}

			// Backward pawn
			if (BitBoard::adjacentFilesForward<OppositeSide>(sq.shift(Up)).b_and(pawns) == BitBoard::EMPTY
				&& BitBoard::pawnAttacks(Side, sq.shift(Up)).b_and(enemyPawns)) {
				entry.pawnEvaluation[Side] += scores::BACKWARD_PAWN;
				entry.backward.set(sq);
			}
		}

		// Pawn islands
		entry.pawnEvaluation[Side] += scores::PAWN_ISLANDS[entry.islandsCount[Side]];

		// Pawn distortion
		entry.pawnEvaluation[Side] += scores::PAWN_DISTORTION * entry.distortion[Side];
	}
}