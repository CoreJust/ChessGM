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

#include "Utils/IO.h"
#include "Chess/BitBoard.h"
#include "Engine/Scores.h"
#include "Engine/Engine.h"

/*
*	main.cpp contains the main function.
* 
*	It does some general initialization, requests the work mode
*	and starts the engine.
* 
*	TODO for the next versions:
*		1) Quiescence search
*		2) Move picking (with SEE)
*		3) History euristic
*		4) Scores optimization
*		5) Tempo bonus
*		6) Review time limits (breaks) calculation
*		7) Extensions
*		8) PV
* 
*	Bizzare ideas:
*		1) Dynamic square's "importance" (center, king zone, piece concentration, etc)
*		   and mobility based on the importance of attacked squares
*		2) Factors beside material in game stage evaluation (for score interpolation)
*		   e.g. pawns advance
* 
*	Current TODO: -
* 
*	Bugs: -
*/

int main() {
	BitBoard::init();
	scores::initScores();
	io::Output::init();
	io::init();

	engine::run(io::getMode());
	
	io::Output::destroy();
	return 0;
}