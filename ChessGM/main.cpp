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

#include "Utils/IO.h"
#include "Chess/BitBoard.h"
#include "Engine/Scores.h"
#include "Engine/Engine.h"
#include "Engine/TranspositionTable.h"
#include "Engine/PawnHashTable.h"

/*
*	main.cpp contains the main function.
* 
*	It does some general initialization, requests the work mode
*	and starts the engine.
* 
*	Bizzare ideas (just some notes):
*		1) Dynamic square's "importance" (center, king zone, piece concentration, etc)
*		   and mobility based on the importance of attacked squares
*		2) Factors beside material in game stage evaluation (for score interpolation)
*		   e.g. pawns advance
* 
*	Canceled ideas (were not useful in the version they were expected to be added in):
*		Countermove Table, SEE in move ordering, Mate Killers, Duo Pawns, Space, Connectivity,
*		Candidate Pawns, Threat Move detection with Null Move, Multicut, Probcut, No pawns, Rook pair
*		
* 
*	TODO by future versions (general plans, features to try):
*		0.8) Extensions (single move extension, check extension, capture/recapture extension,
*						 passed pawn extension, PV extension, singular extension...)
*		0.9) King update (mate at a glance, pins/x-rays, castlings (rights), pawn shield, pawn storm, tropism,
*						   virtual mobility, scaling with material, king zone attack, square control in king zone,
*						   king pawn tropism)
*		0.10) General endgame evaluation and search improvement, parallel search
*		1.0) Final improvements before the first release: some small additions, optimization, bugs fixing...)
*		1.1) Evaluation weights search via learning
*		1.2) ???
* 
*	Tips for another minor releases - things to be done before pushing to the git:
*		1) Running all the tests
*		2) Elo search with no less than 300 games (the higher the version - the more games are required)
*		3) Test run in all the three modes - Xboard, UCI, console
*		4) Ensuring correct Makefile
*		5) Ensuring all the changes were properly documented in changelog.txt and the readme.md was properly modified
* 
*	Current version TODO: 
*		0.7) Knights and bishops (bad bishop, fianchetto, color weakness),
*			 Rooks and queens (rook on (semi)open file, rook on seventh rank,
*							   paired rooks, queens penalty for early development, tropism?)
* 
*	Bugs: -
*/

int main() {
	BitBoard::init();
	scores::initScores();
	engine::TranspositionTable::init();
	engine::PawnHashTable::init();
	io::Output::init();
	io::init();

	engine::run(io::getMode());
	
	io::Output::destroy();
	engine::TranspositionTable::destroy();
	return 0;
}