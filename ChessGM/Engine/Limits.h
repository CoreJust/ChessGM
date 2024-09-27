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

#pragma once
#include "Utils/Types.h"

/*
*	Limits(.h/.cpp) contains class Limits that is responsible 
*	for limitins the search.
* 
*	There are three possible limitations: by time, by maximal root depth, and by nodes.
*/

namespace engine {
	// Limits the search
	class Limits final {
	private:
		time_t m_softBreak = INT64_MAX;
		time_t m_hardBreak = INT64_MAX;
		time_t m_start = 0;

		u32 m_timeControlMoves = 0;
		i32 m_movesMade = 0;
		time_t m_baseTime = 60000;
		time_t m_incTime = 3000;
		Depth m_depthLimit = 99;
		NodesCount m_nodesLimit = UINT64_MAX;

	public:
		// Resets all the limits and makes the search infinite
		void makeInfinite() noexcept;

		// Starts the time limit counting from the moment
		// Recomputes the limits based on the currently left time
		void reset(const time_t msLeft = 0) noexcept;

		// Accounts the number of moves made (or unmade for negative numbers)
		void addMoves(const i32 cnt) noexcept;

		// Conventional time control is when there is some amount of time that gets reset once in a control
		// Control is a number of moves
		void computeConventionalTimeLimits(const time_t msLeft) noexcept;

		// Incremental time control is there is some time in base - it is not being reset
		// But after every move the side is given some additional amount of time
		void computeIncrementalTimeLimits(const time_t msLeft) noexcept;

		// If an exact number of seconds is given per move, then there is no base
		// Time does not accumulate in this control
		void computeExactTimePerMove(const time_t msLeft) noexcept;

		void setTimeLimits(const u32 control, const u32 secondsBase, const u32 secondsInc);
		void setTimeLimitsInMs(const u32 control, const time_t secondsBase, const time_t secondsInc);
		void setNodesLimit(const NodesCount nodes) noexcept;
		void setDepthLimit(const Depth depth) noexcept;

		time_t elapsedCentiseconds() const noexcept;
		time_t elapsedMilliseconds() const noexcept;

		// Soft limit is the optimal time to end the search
		// The search is stopped if the soft limit is broken in a convenient time
		bool isSoftLimitBroken() const noexcept;

		// Hard limit is the time when the search is stopped no matter what
		bool isHardLimitBroken() const noexcept;

		bool isNodesLimitBroken(const NodesCount nodes) const noexcept;
		bool isDepthLimitBroken(const Depth depth) const noexcept;
	};
}