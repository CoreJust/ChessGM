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

#include "Limits.h"
#include <chrono>

namespace engine {
	time_t timeNow() noexcept {
		auto _now = std::chrono::steady_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(_now).count();
	}

	void Limits::reset(const time_t msLeft) noexcept {
		// DELAY_FIX is the time that is reserved due to the delays
		// that are not accounted by the time limits
		// It is the time elapsed before the reset() was called
		// and some reserve considering that there can be a slight delay
		// between running out of time and passing the search result on 
		// to the GUI
		constexpr time_t DELAY_FIX = 2;

		m_start = timeNow() - DELAY_FIX;
		if (m_incTime == 0) {
			computeConventionalTimeLimits(msLeft);
		} else if (m_timeControlMoves == 0) {
			computeIncrementalTimeLimits(msLeft);
		} else {
			computeExactTimePerMove(msLeft);
		}
	}

	void Limits::addMoves(const i32 cnt) noexcept {
		if (m_timeControlMoves) {
			m_movesMade += cnt;
			m_movesMade %= m_timeControlMoves;
		}
	}

	void Limits::computeConventionalTimeLimits(const time_t msLeft) noexcept {
		const time_t msPerMove = msLeft
			? msLeft / (m_timeControlMoves - m_movesMade)
			: m_baseTime / m_timeControlMoves;

		m_softBreak = m_start + msPerMove / 2;
		m_hardBreak = m_start + time_t(msPerMove * 0.9);
	}

	void Limits::computeIncrementalTimeLimits(const time_t msLeft) noexcept {
		constexpr u32 GAME_LENGTH_FACTOR = 40;

		const time_t msPerMove = msLeft >= m_incTime + m_baseTime / GAME_LENGTH_FACTOR
			? (msLeft / GAME_LENGTH_FACTOR)
			: m_incTime + m_baseTime / GAME_LENGTH_FACTOR;

		m_softBreak = m_start + msPerMove / 2;
		m_hardBreak = m_start + time_t(msPerMove * 0.9);
	}

	void Limits::computeExactTimePerMove(const time_t msLeft) noexcept {
		const time_t msForMove = msLeft ? msLeft : m_incTime;
		m_softBreak = m_start + time_t(msForMove * 0.88);
		m_hardBreak = m_start + time_t(msForMove * 0.92);
	}

	void Limits::setTimeLimits(const u32 control, const u32 secondsBase, const u32 secondsInc) {
		m_timeControlMoves = control;
		m_baseTime = time_t(secondsBase) * 1000;
		m_incTime = time_t(secondsInc) * 1000;
	}

	void Limits::setTimeLimitsInMs(const u32 control, const time_t secondsBase, const time_t secondsInc) {
		m_timeControlMoves = control;
		m_baseTime = secondsBase;
		m_incTime = secondsInc;
	}

	void Limits::setNodesLimit(const NodesCount nodes) noexcept {
		m_nodesLimit = nodes;
	}

	void Limits::setDepthLimit(const Depth depth) noexcept {
		m_depthLimit = depth;
	}

	time_t Limits::elapsedCentiseconds() const noexcept {
		return (timeNow() - m_start) / 10;
	}

	time_t Limits::elapsedMilliseconds() const noexcept {
		return timeNow() - m_start;
	}

	bool Limits::isSoftLimitBroken() const noexcept {
		return timeNow() >= m_softBreak;
	}

	bool Limits::isHardLimitBroken() const noexcept {
		return timeNow() >= m_hardBreak;
	}

	bool Limits::isNodesLimitBroken(const NodesCount nodes) const noexcept {
		return nodes > m_nodesLimit;
	}

	bool Limits::isDepthLimitBroken(const Depth depth) const noexcept {
		return depth > m_depthLimit;
	}
}