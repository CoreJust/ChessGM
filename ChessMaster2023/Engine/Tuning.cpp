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

#include "Tuning.h"
#include <cmath>
#include <random>
#include <fstream>
#include <iomanip>

#include "Utils/StringUtils.h"
#include "Utils/IO.h"
#include "Eval.h"
#include "PawnHashTable.h"

namespace engine {
    void extractHeader(std::ifstream& pgn, std::string& initialFen, float &result) {
        std::string line;

        initialFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"; // FEN by default
        while (std::getline(pgn, line) && (line[0] == '[' || line.size() < 2)) {
            if (line.starts_with("[Result")) {
                if (line == "[Result \"1/2-1/2\"]") {
                    result = 0.5f;
                } else if (line == "[Result \"0-1\"]") {
                    result = 0.f;
                } else {
                    result = 1.f;
                }
            } else if (line.starts_with("[FEN")) {
                const size_t fenTo = line.find_last_of('"');
                initialFen = line.substr(6, fenTo - 6);
            }
        }
    }

    void skipTillMoves(std::ifstream& pgn, std::string& line) {
        while (std::getline(pgn, line) && line[0] != '1');
    }

    void extractMoves(
        std::ifstream& pgn, 
        std::string& line,
        u32& movesCount,
        const std::string& initialFen,
        std::vector<std::string>& fens,
        std::vector<u32>& fenMoveCounter
    ) {
        std::vector<std::string_view> moves;
        bool success;
        Board board = Board::fromFEN(initialFen, success);

        fens.clear();
        fenMoveCounter.clear();
        movesCount = 0;
        if (!success) {
            return;
        }

        bool wasQuiet = true; // Was the previous move quiet?
        do {
            moves = str_utils::split(line, ". \n\t", std::move(moves));

            for (auto& moveStr : moves) {
                if (moveStr == "{") {
                    std::getline(pgn, line);
                    return;
                }

                if (std::isalpha(moveStr[0])) {
                    Move m = board.makeMoveFromString(moveStr);
                    if (m.isNullMove() || !board.isLegal(m)) {
                        return;
                    }

                    if (!board.givesCheck(m) && board.isQuiet(m)) {
                        if (!board.isInCheck() && wasQuiet) {
                            fens.push_back(board.toFEN());
                            fenMoveCounter.push_back(movesCount);
                        }

                        wasQuiet = true;
                    } else {
                        wasQuiet = false;
                    }

                    ++movesCount;
                    board.makeMove(m);
                }
            }
        } while (std::getline(pgn, line) && line.size() > 1);
    }

    void Tuning::extractPositions(const std::string& pgnFileName, const std::string& positionsFileName) {
        constexpr u32 FENS_PER_GAME = 5; // How much positions to extract from a single game

        if (pgnFileName == positionsFileName) {
            return;
        }

        std::ifstream pgn(pgnFileName);
        std::ofstream out(positionsFileName);
        std::vector<std::string> fens;
        std::vector<u32> fenMoveCounters;
        i32 indices[FENS_PER_GAME]; // indices of the positions chosen in the game
        std::string initialFen;
        std::string line;
        float result;
        u32 movesCount;

        while (!pgn.eof() && pgn.is_open() && pgn.good()) {
            extractHeader(pgn, initialFen, result);
            skipTillMoves(pgn, line);
            extractMoves(pgn, line, movesCount, initialFen, fens, fenMoveCounters);

            u32 step = (fens.size() <= FENS_PER_GAME) ? 1 : (fens.size() / FENS_PER_GAME);
             
            for (u32 i = 0; i < fens.size(); i += step) {
                u32 len = movesCount - fenMoveCounters[i];
                out << fens[i] << " res " << result << "; len " << len << ";" << std::endl;
            }
        }
    }

    void Tuning::loadPositions(const std::string& fileName) {
        std::ifstream file(fileName);
        std::string line;

        while (std::getline(file, line)) {
            size_t resPos = line.find("res");
            std::string fen = line.substr(0, resPos - 1);
            float result = line[resPos + 4] == '1' 
                    ? 1.f 
                : line[resPos + 6] == '5' 
                    ? 0.5f 
                    : 0.f;
            
            bool success;
            if (Board board = Board::fromFEN(fen, success); success) {
                m_positions.emplace_back(Position { std::move(board), result });
            }
        }
    }

    void Tuning::optimizeScores(const std::vector<Value*>& scores, u32 iterationsCount) {
        double err = computeErr();
        io::g_out << "Tuning begins, initial error: " << io::Color::Cyan << std::setprecision(10) << err << std::endl;

        io::g_out << "Weights:" << std::endl;
        for (Value* pScore : scores) {
            io::g_out << "\t" << *pScore << std::endl;
        }

        for (u32 iteration = 0; iteration < iterationsCount; iteration++) {
            double iterationInitialErr = err;

            for (Value* pScore : scores) {
                Value& score = *pScore;

                // Trying to change the score for a step, decreasing the step on failure
                Value step = iteration 
                    ? (iteration == 1 
                       ? 8 
                       : 1) 
                    : 32;

                while (step) {
                    // A step higher
                    score += step;
                    double tmp = computeErr();
                    if (tmp < err) {
                        err = tmp;
                        continue;
                    }

                    // A step lower
                    score -= step * 2;
                    tmp = computeErr();
                    if (tmp < err) {
                        err = tmp;
                        continue;
                    }

                    // Recovering the original score and decreasing the step
                    score += step;
                    step /= 2;
                }
            }

            if (err == iterationInitialErr) {
                break; // No changes
            }

            io::g_out << "Iteration " << io::Color::Blue << (iteration + 1) << io::Color::White
                << ", error: " << io::Color::Cyan << std::setprecision(10) << err << "(-" << (iterationInitialErr - err) 
                << ")" << std::endl;

            io::g_out << "Weights:" << std::endl;
            for (Value* pScore : scores) {
                io::g_out << "\t" << *pScore << std::endl;
            }
        }
    }

    double Tuning::computeErr() {
        double result = 0.0;
        size_t n = 0;

        PawnHashTable::reset();

        for (Position& pos : m_positions) {
            ++n;
            Value staticEval = eval(pos.board);
            staticEval = pos.board.side() == Color::WHITE ? staticEval : -staticEval; // Always consider from white POV

            // The expected result probability  
            const double resultProbability = 1.0 / (1.0 + exp(-staticEval / 190.0));
            const double error = resultProbability - pos.result;

            result += error * error;
        }

        result /= n;
        return sqrt(result);
    }
}