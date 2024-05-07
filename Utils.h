#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>

class Utils
{
public:
    Utils();

    static std::string timeFormat(int64_t seconds);
    static std::string byteFormat(int64_t bytes);
};

void Log(const std::string& msg);

// class Log
// {
// public:
//     void operator()(const std::string& msg) {
//         std::cout << msg << std::endl;
//     }
// };

#endif // UTILS_H
