/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   main_timer.cpp                                             _             */
/*                                                            _ \'-_,#        */
/*   By: jodos-sa <jodos-sa@student.42porto.com>             _\'--','`|       */
/*                                                           \`---`  /        */
/*   Created: 2026/09/24 17:36:40 by jodos-sa                 `----'`         */
/*   Updated: 2026/09/24 17:43:13 by jodos-sa                                 */
/*                                                                            */
/* ************************************************************************** */

#include "timer.hpp"
#include <chrono>
#include <thread>
#include <iostream>

int main(){
	Timer timer(std::chrono::milliseconds(60000));

	timer.start();
	std::cout << timer.timedOut() << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(65000));
	std::cout << timer.timedOut() << std::endl;

	timer.reset();
	std::cout << timer.timedOut() << std::endl;
	
}
