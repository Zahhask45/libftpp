#include "chronometer.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    Chronometer chronometer;

    chronometer.start();
    std::cout << chronometer.elapsed().count() << " ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    chronometer.stop();

    std::cout << chronometer.elapsed().count() << " ms" << std::endl;

    chronometer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    chronometer.stop();

    std::cout << chronometer.elapsed().count() << " ms" << std::endl;

    chronometer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    chronometer.stop();
    std::cout << chronometer.elapsed().count() << " ms" << std::endl;
    
    chronometer.reset();
    chronometer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    chronometer.stop();
    std::cout << chronometer.elapsed().count() << " ms" << std::endl;
    chronometer.reset();

    if (chronometer.elapsed().count() != 0)
        return 1;

    return 0;
}
