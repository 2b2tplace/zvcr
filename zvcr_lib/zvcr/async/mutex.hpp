#pragma once

#include <mutex>

namespace zvcr::mutex {

    template<class T, class M=std::mutex,
             template<class...> class WL=std::unique_lock,
             template<class...> class RL=std::unique_lock>
    struct Mutex {
        auto read(auto readFunction) const {
            const auto lock = readLock();
            return readFunction(data);
        }

        template <typename Member>
        Member read(Member T::*member) const {
            const auto lock = readLock();
            return data.*member;
        }

        auto write(auto writeFunction) {
            const auto lock = writeLock();
            return writeFunction(data);
        }

        T clone() const {
            return read([this](const auto&) { return data; });
        }

        Mutex() = default;
        explicit Mutex(T in): data(std::move(in)) {}
    private:
        mutable M mtx;
        T data;

        auto readLock() const {
            return RL<M>(mtx);
        }

        auto writeLock() const {
            return WL<M>(mtx);
        }
    };

}
