#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdio.h>
#include <chrono>

struct Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> previous_timestamp;

    void start() { previous_timestamp = std::chrono::high_resolution_clock::now(); }

    float tick() {
        auto now = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - previous_timestamp).count();
        previous_timestamp = now;
        // if (ms > 0) {
        //     std::cout << "fps: " << (1000.0/ms) << std::endl;
        // }
        return ms / 1000.f;
    }

    //     void sync(int ms) {
    //         auto now = std::chrono::high_resolution_clock::now();
    //         auto elapsed = now - previous_timestamp;
    //         if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < ms) {
    //             std::this_thread::sleep_for(std::chrono::milliseconds(ms) - elapsed);
    //         }
    //         previous_timestamp = now;
    //     }
};

struct Period {
    int period_ms = 1000;
    int ms_accumulated = 0;

    bool passed(float delta_seconds) {
        ms_accumulated += delta_seconds * 1000;
        if (ms_accumulated > period_ms) {
            ms_accumulated = 0;
            return true;
        }
        return false;
    }
};

Period period_every_hour() {
    return {.period_ms = 3600 * 1000};
}

inline float randf() {
    return (float)rand() / (float)RAND_MAX;
}

#endif /* UTIL_HPP */
