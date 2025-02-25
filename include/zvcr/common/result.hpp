#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

#define Try(expr) ({ auto __tmp_expect = (expr); if (!__tmp_expect.ok()) return Error(__tmp_expect.error()); __tmp_expect.unwrap(); })
#define Propagate(expr) ({ if (const auto __tmp_err = (expr); __tmp_err.hasSome()) return Error(__tmp_err.unwrap()); })

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
        const T& unwrap() const {
            if (!ok()) throw std::runtime_error("Trying to access non-existent value in Result<T, E>");
            return std::get<T>(valueOrError);
        }

        [[nodiscard]]
        T* operator->() const {
            return &unwrap();
        }

        [[nodiscard]]
        const E& error() const {
            if (ok()) throw std::runtime_error("Trying to access non-existent error in Result<T, E>");
            return std::get<Error<E>>(valueOrError).get();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!ok()) return defaultValue;
            return unwrap();
        }

        template<class U>
        Result<U, E> andThen(std::function<U(const T&)> func) {
            if (!ok()) return Error(error());
            return Result<U, E>(func(unwrap()));
        }

        template<class F, typename U = decltype(std::declval<F>()(std::declval<T>()))>
        Result<U, E> andThen(F func) const {
            std::function f = std::forward<F>(func);
            return andThen(f);
        }
    };

    template <typename T>
    class Option {
        std::optional<T> valueOrEmpty;
    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        Option(T&& value) : valueOrEmpty(std::optional<T>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Option(const T& value) : valueOrEmpty(std::optional<T>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Option() : valueOrEmpty(std::nullopt) {} // NOLINT(*-explicit-constructor)

        [[nodiscard]]
        bool hasSome() const {
            return valueOrEmpty.has_value();
        }

        [[nodiscard]]
        const T& unwrap() const {
            if (!hasSome()) throw std::runtime_error("Trying to access non-existent value in Option<T>");
            return valueOrEmpty.value();
        }

        [[nodiscard]]
        const T* operator->() const {
            return &unwrap();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!hasSome()) return defaultValue;
            return unwrap();
        }

        template<class U>
        Option<U> andThen(std::function<U(const T&)> func) {
            if (!hasSome()) return Option<U>();
            return Option<U>(func(unwrap()));
        }

        template<class F, typename U = decltype(std::declval<F>()(std::declval<T>()))>
        Option<U> andThen(F func) const {
            std::function f = std::forward<F>(func);
            return andThen(f);
        }

    };

}