/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   timer.hpp                                                  _             */
/*                                                            _ \'-_,#        */
/*   By: jodos-sa <jodos-sa@student.42porto.com>             _\'--','`|       */
/*                                                           \`---`  /        */
/*   Created: 2026/09/24 17:10:35 by jodos-sa                 `----'`         */
/*   Updated: 2026/09/24 17:34:45 by jodos-sa                                 */
/*                                                                            */
/* ************************************************************************** */

#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

class Timer {
	public:
		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::milliseconds;

		explicit Timer(Duration duration):
			_duration(duration),
			_startTime(),
			_running(false){}

		void start(){
			_startTime = Clock::now();
			_running = true;
		}

		void reset(){
			_running = false;
			_startTime = Clock::time_point();
		}

		bool timedOut() const{
			if (!_running)
				return false;

			return Clock::now() - _startTime >=  _duration;
		}

		bool isRunning() const{
			return _running;
		}

		Duration duration() const{
			return _duration;
		}

		
	private:
		Duration _duration;
		Clock::time_point _startTime;
		bool _running;
};

#endif
