/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   chronometer.hpp                                            _             */
/*                                                            _ \'-_,#        */
/*   By: jodos-sa <jodos-sa@student.42porto.com>             _\'--','`|       */
/*                                                           \`---`  /        */
/*   Created: 2026/09/24 17:48:25 by jodos-sa                 `----'`         */
/*   Updated: 2026/09/24 17:57:31 by jodos-sa                                 */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHRONOMETER
#define CHRONOMETER

#include <chrono>

class Chronometer{
	public:
		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::milliseconds;

		Chronometer():
			_elapsed(0),
			_startTime(),
			_running(false){}

		void start(){
			if (_running)
				return;
			_startTime = Clock::now();
			_running = true;
		}

		void stop(){
			if (!_running)
				return ;
			_elapsed += currentLap();
			_running = false;
		}

		void reset(){
			_elapsed = Duration(0);
			_startTime = Clock::time_point();
			_running = false;
		}

		Duration elapsed() const{
			if (!_running)
				return _elapsed;

			return _elapsed + currentLap();
		}

		bool isRunning() const{
			return _running;
		}

	private:
		Duration currentLap() const{
			return std::chrono::duration_cast<Duration>(Clock::now() - _startTime);
		}
		
		Duration _elapsed;
		Clock::time_point _startTime;
		bool _running;
};

#endif
