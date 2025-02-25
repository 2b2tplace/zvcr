#pragma once

#include <stdexcept>
#include <utility>
#include <variant>

#define Try(expr) ({ auto __tmp_expect = (expr); if (!__tmp_expect.ok()) return Error(__tmp_expect.error()); __tmp_expect.value(); })
#define Propagate(expr) ({ if (const auto __tmp_err = (expr); __tmp_err.has_value()) return Error(__tmp_err.value()); })

namespace zvcr::common::result {

    template<typename E>
    struct Error {
        E error;

        explicit Error(E&& error) : error(std::forward<E>(error)) {}

        explicit Error(const E&) : error(std::forward<E>(error)) {}

        Error(const Error&) = default;

        Error(Error&&) = default;

        [[nodiscard]]
        const E& get() const {
            return error;
        }
    };

    template <typename T, typename E>
    class Result {
        std::variant<T, Error<E>> valueOrError;
    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(T&& value) : valueOrError(std::variant<T, Error<E>>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(const T& value) : valueOrError(std::variant<T, Error<E>>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(const Error<E>& error) : valueOrError(std::variant<T, Error<E>>{error}) {} // NOLINT(*-explicit-constructor)

        [[nodiscard]]
        bool ok() const {
            return std::holds_alternative<T>(valueOrError);
        }

        [[nodiscard]]
        const T& value() const {
            if (!ok()) throw std::runtime_error("Trying to access non-existent value in Result<T, E>");
            return std::get<T>(valueOrError);
        }

        [[nodiscard]]
        const E& error() const {
            if (ok()) throw std::runtime_error("Trying to access non-existent error in Result<T, E>");
            return std::get<Error<E>>(valueOrError).get();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!ok()) return defaultValue;
            return value();
        }
    };

}