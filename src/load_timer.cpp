#include "load_timer.h"

#include "raylib.h"

LoadTimer::LoadTimer(const char* name)
    : name(name),
      startTime(std::chrono::steady_clock::now())
{
}

LoadTimer::~LoadTimer()
{
    const auto endTime = std::chrono::steady_clock::now();

    const float elapsedMilliseconds =
        std::chrono::duration<float, std::milli>(
            endTime - startTime
        ).count();

    TraceLog(
        LOG_INFO,
        "LOAD TIMER: %s took %.2f ms",
        name,
        elapsedMilliseconds
    );
}