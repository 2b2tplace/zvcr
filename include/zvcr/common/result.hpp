#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

#define Try(expr) ({ auto __tmp_expect = (expr); if (__tmp_expect.error()) return Error(__tmp_expect.unwrap_error()); __tmp_expect.unwrap(); })
#define Propagate(expr) ({ if (const auto __tmp_err = (expr); __tmp_err.some()) return Error(__tmp_err.unwrap()); })
#define Require(expr) ({ auto __tmp_expect_some = (expr); if (__tmp_expect_some.none()) return {}; __tmp_expect_some.unwrap(); })

namespace zvcr::common::result {

    template<typename E>
    struct Error {
        E error;

        explicit Error(E&& error) : error(std::move(error)) {}

        explicit Error(const E& error) : error(error) {}

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
        Result(T&& value) : valueOrError(std::variant<T, Error<E>>{std::move(value)}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(const T& value) : valueOrError(std::variant<T, Error<E>>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(const Error<E>& error) : valueOrError(std::variant<T, Error<E>>{error}) {} // NOLINT(*-explicit-constructor)

        [[nodiscard]]
        bool ok() const {
            return std::holds_alternative<T>(valueOrError);
        }

        [[nodiscard]]
        bool error() const {
            return !ok();
        }

        [[nodiscard]]
        const T& unwrap() const {
            return expect("Trying to access non-existent value in Result<T, E>");
        }

        [[nodiscard]]
        const T& expect(const std::string& orError) const {
            if (!ok()) throw std::runtime_error(orError);
            return std::get<T>(valueOrError);
        }

        [[nodiscard]]
        const T* operator->() const {
            return &unwrap();
        }

        [[nodiscard]]
        const E& unwrap_error() const {
            return expect_error("Trying to access non-existent error in Result<T, E>");
        }

        [[nodiscard]]
        const E& expect_error(const std::string& orError) const {
            if (ok()) throw std::runtime_error(orError);
            return std::get<Error<E>>(valueOrError).get();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!ok()) return defaultValue;
            return unwrap();
        }

        template<class U>
        Result<U, E> andThen(std::function<U(const T&)> func) {
            if (!ok()) return Error(unwrap_error());
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
        Option(T&& value): valueOrEmpty(std::optional<T>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Option(const T& value): valueOrEmpty(std::optional<T>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Option(): valueOrEmpty(std::nullopt) {} // NOLINT(*-explicit-constructor)

        [[nodiscard]]
        bool some() const {
            return valueOrEmpty.has_value();
        }

        [[nodiscard]]
        bool none() const {
            return !some();
        }

        [[nodiscard]]
        const T& unwrap() const {
            return expect("Trying to access non-existent value in Option<T>");
        }

        [[nodiscard]]
        const T& expect(const std::string& orError) const {
            if (!some()) throw std::runtime_error(orError);
            return valueOrEmpty.value();
        }

        [[nodiscard]]
        const T* operator->() const {
            return &unwrap();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!some()) return defaultValue;
            return unwrap();
        }

        template<class U>
        Option<U> andThen(std::function<U(const T&)> func) {
            if (!some()) return Option<U>();
            return Option<U>(func(unwrap()));
        }

        template<class F, typename U = decltype(std::declval<F>()(std::declval<T>()))>
        Option<U> andThen(F func) const {
            std::function f = std::forward<F>(func);
            return andThen(f);
        }

    };

    template <typename T>
    class OptionRef {
        Option<std::reference_wrapper<T>> option;
    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        OptionRef(T&& value): option(value) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        OptionRef(const T& value): option(value) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        OptionRef(): option() {} // NOLINT(*-explicit-constructor)

        [[nodiscard]]
        bool some() const {
            return option.some();
        }

        [[nodiscard]]
        bool none() const {
            return !some();
        }

        [[nodiscard]]
        T& unwrap() const {
            return option.unwrap().get();
        }

        [[nodiscard]]
        T& expect(const std::string& orError) const {
            return option.expect(orError).get();
        }

        [[nodiscard]]
        T* operator->() const {
            return &unwrap();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!some()) return defaultValue;
            return unwrap();
        }

    };

    template <typename T>
    using OptionCRef = OptionRef<const T>;

}