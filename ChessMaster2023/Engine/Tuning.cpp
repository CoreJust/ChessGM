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
#include <fstream>

#include "Eval.h"

namespace engine {
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

            m_positions.push_back(Position { std::move(fen), result });
        }
    }

    double Tuning::computeErr() {
        double result = 0.0;
        size_t n = 0;
        Board board;

        for (Position& pos : m_positions) {
            bool success;
            board = Board::fromFEN(pos.fen, success);
            if (success) {
                ++n;
                Value staticEval = eval(board);
                staticEval = board.side() == Color::WHITE ? staticEval : -staticEval; // Always consider from white POV

                // The expected result probability  
                const double resultProbability = 1.0 / (1.0 + exp(-staticEval / 190.0));
                const double error = resultProbability - pos.result;

                result += error * error;
            }
        }

        result /= n;
        return sqrt(result);
    }
}