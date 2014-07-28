#include <chrono>

#include "accessor.hpp"
#include "game_time.hpp"

GameTime::GameTime():
    game_seconds_per_real_second(
        1,
        Accessor<double>::default_getter,
        [this] (double value) { time(); return value; }
    ),
    passed_time(duration(0)),
    time_at_last_tick(std::chrono::steady_clock::now())
    {}

GameTime::GameTime(const GameTime &other):
    game_seconds_per_real_second(
        other.game_seconds_per_real_second,
        Accessor<double>::default_getter,
        [this] (double value) { time(); return value; }
    ),
    passed_time(other.passed_time),
    time_at_last_tick(other.time_at_last_tick)
    {}

GameTime::time_point GameTime::time() {
    auto real_time = std::chrono::steady_clock::now();
    auto real_time_difference = real_time - time_at_last_tick;

    passed_time += std::chrono::duration_cast<duration>(
        real_time_difference * double(game_seconds_per_real_second)
    );
    time_at_last_tick = real_time;

    // Make it seem like this is a time.
    return time_point(passed_time);
}
