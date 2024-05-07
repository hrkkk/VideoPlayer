#include "Utils.h"
#include <mutex>

Utils::Utils() {}

// extern std::mutex mtx;

void Log(const std::string& msg)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::cout << msg << std::endl;
}

std::string Utils::timeFormat(int64_t totalSeconds)
{
    int hour = totalSeconds / (60 * 60);
    totalSeconds %= (60 * 60);
    int minute = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    std::string formatStr = hour > 0 ? std::to_string(hour) + " h " : "";
    formatStr += minute > 0 ? std::to_string(minute) + " m " : "";
    formatStr += std::to_string(seconds) + " s";

    return formatStr;
}

std::string Utils::byteFormat(int64_t bytes)
{
    int gb = bytes / (1024 * 1024 * 1024);
    bytes %= (1024 * 1024 * 1024);
    int mb = bytes / (1024 * 1024);
    bytes %= (1024 * 1024);
    int kb = bytes / 1024;
    int b = bytes % 1024;

    std::string formatStr = gb > 0 ? std::to_string(gb) + " GB " : "";
    formatStr += mb > 0 ? std::to_string(mb) + " MB " : "";
    formatStr += kb > 0 ? std::to_string(kb) + " KB " : "";
    formatStr += std::to_string(b) + " B";

    return formatStr;
}
