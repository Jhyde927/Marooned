#pragma once

#include <chrono>

class LoadTimer
{
public:
    explicit LoadTimer(const char* name);
    ~LoadTimer();

private:
    const char* name;
    std::chrono::steady_clock::time_point startTime;
};