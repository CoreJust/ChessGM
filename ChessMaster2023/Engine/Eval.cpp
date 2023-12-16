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

#include "Eval.h"

Value engine::eval(Board& board) {
    Score score = board.scoreByColor(Color::WHITE) - board.scoreByColor(Color::BLACK);

    ///  RESULTS  ///

    Material material = board.materialByColor(Color::WHITE) + board.materialByColor(Color::BLACK);
    Value result = score.collapse(material);
    result *= (-1 + 2 * (board.side() == Color::WHITE));

    return result + scores::TEMPO_SCORE.collapse(material);
}
