#include "states/main_descent.h"
#include "HAL/time.h"
#include "scheduler.hpp"

StateId State::MainDescent::getNewState(const StateInput &input,
                                        StateAuxilliaryInfo &) {
    static float prev_altitude = input.altitude;
    static auto prev_time = Hal::now_ms(); // time_point
    static auto curr_time = Hal::now_ms();
    static uint8_t num_checks = 0;

    typedef std::chrono::milliseconds ms;

    if (curr_time - prev_time >= LANDED_TIME_INTERVAL_) {
        auto dt_ms =
            std::chrono::duration_cast<ms>(curr_time - prev_time).count();
        if (abs(input.altitude - prev_altitude) * 1000 / dt_ms <
            LANDED_VELOCITY_) {
            num_checks++;
        } else {
            num_checks = 0;
        }

        if (num_checks < LANDED_CHECKS_) {
            return StateId::MAIN_DESCENT;
        }
        prev_time = curr_time;
    }

    return landed_state_;
}
