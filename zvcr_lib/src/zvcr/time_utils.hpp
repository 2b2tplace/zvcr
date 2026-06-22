#pragma once

#include <ranges>
#include <cmath>
#include <ctime>

namespace zvcr {

    template<std::ranges::input_range TimestampSource, typename TimestampMapFn>
    auto findNearestTimestamp(const TimestampSource &source, const TimestampMapFn &fn,
                              const time_t compareToTimestamp) -> time_t {
        auto closest = compareToTimestamp;
        bool found = false;
        time_t minDistance{};

        for (const auto candidate : source) {
            const time_t candidateTimestamp = fn(candidate);
            const auto distance = std::abs(candidateTimestamp - compareToTimestamp);

            if (!found || distance < minDistance) {
                found = true;
                minDistance = distance;
                closest = candidateTimestamp;
            }
        }
        return closest;
    }

    template<std::ranges::input_range TimestampSource>
    auto findNearestTimestamp(const TimestampSource &source, const time_t compareToTimestamp) -> time_t {
        return findNearestTimestamp(
            source,
            [](const auto &snapshot) {
                return snapshot.timestamp;
            },
            compareToTimestamp
        );
    }

}
