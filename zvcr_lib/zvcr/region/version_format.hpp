#pragma once

#include <zvcr/region/file_location.hpp>
#include <zvcr/region/dim2/zvcr2.hpp>
#include <zvcr/region/dim3/zvcr3.hpp>

namespace zvcr {

    template<typename R>
    struct VersionToString {
        static_assert(std::is_same_v<R, ZVCR2File> || std::is_same_v<R, ZVCR3File>,
            "ZVCR VersionToString only supports ZVCR2File and ZVCR3File");
    };

    template<>
    struct VersionToString<ZVCR2File> {
        static constexpr auto versionName = versionName2;
        static constexpr auto extension = "zvcr2";
    };

    template<>
    struct VersionToString<ZVCR3File> {
        static constexpr auto versionName = versionName3;
        static constexpr auto extension = "zvcr3";
    };

}
