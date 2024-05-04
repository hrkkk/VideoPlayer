#include "Utils.h"
#include <mutex>

Utils::Utils() {}

// extern std::mutex mtx;

void Log(const std::string& msg)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::cout << msg << std::endl;
}
