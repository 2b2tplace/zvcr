#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

#define Err(error) zvcr::common::result::Error(error)

#define Try(expr) ({                                \
    const auto& __tmp_expect = (expr);              \
    if (__tmp_expect.error())                       \
        return Err(__tmp_expect.unwrapError());     \
                                                    \
    __tmp_expect.unwrap();                          \
})

#define Propagate(expr) ({                                  \
    if (const auto& __tmp_err = (expr); __tmp_err.some())   \
        return Err(__tmp_err.unwrap());                     \
})

#define PropagateVal(expr) ({                                   \
    if (const auto& __tmp_err = (expr); __tmp_err.some())       \
        return __tmp_err;                                       \
})

#define Require(expr) ({                      \
    const auto& __tmp_expect_some = (expr);   \
    if (__tmp_expect_some.none())             \
        return {};                            \
                                              \
    __tmp_expect_some.unwrap();               \
})

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

    template<typename T, typename E>
    class Result {
        std::variant<T, Error<E>> valueOrError;
    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(T&& value): valueOrError(std::variant<T, Error<E>>{std::move(value)}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(const T& value): valueOrError(std::variant<T, Error<E>>{value}) {} // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Result(const Error<E>& error): valueOrError(std::variant<T, Error<E>>{error}) {} // NOLINT(*-explicit-constructor)

        [[nodiscard]]
        bool ok() const {
            return std::holds_alternative<T>(valueOrError);
        }

        [[nodiscard]]
        bool error() const {
            return !ok();
        }

        [[nodiscard]]
        bool none() const {
            return error();
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
        const E& unwrapError() const {
            return expectError("Trying to access non-existent error in Result<T, E>");
        }

        [[nodiscard]]
        const E& expectError(const std::string& orError) const {
            if (ok()) throw std::runtime_error(orError);
            return std::get<Error<E>>(valueOrError).get();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (!ok()) return defaultValue;
            return unwrap();
        }

    };

    template<typename T, typename T_inner = T, typename T_ptr = const T*, typename T_ref = const T&>
    class Option {
        std::optional<T_inner> valueOrEmpty;
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
        T_ref unwrap() const {
            return expect("Trying to access non-existent value in Option<T>");
        }

        [[nodiscard]]
        T_ref expect(const std::string& orError) const {
            if (!some()) throw std::runtime_error(orError);
            return valueOrEmpty.value();
        }

        [[nodiscard]]
        T_ptr operator->() const {
            return &unwrap();
        }

        [[nodiscard]]
        const T& orElse(const T& defaultValue) const {
            if (none()) return defaultValue;
            return unwrap();
        }

        [[nodiscard]]
        const Option& orElse(const Option& otherOption) const {
            if (none()) return otherOption;
            return *this;
        }

        template<typename E>
        [[nodiscard]]
        Result<T_inner, E> okOr(const E& error) const {
            if (none()) return Err(error);
            return unwrap();
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
            if (none()) return defaultValue;
            return unwrap();
        }

    };

    template <typename T>
    using OptionCRef = OptionRef<const T>;

    template<typename T, typename E>
    std::ostream& operator<<(std::ostream& os, const Result<T, E>& result) {
        static constexpr std::string_view OK_PREFIX = "Ok(";
        static constexpr std::string_view ERR_PREFIX = "Err(";

        if (result.ok())
            return os << OK_PREFIX << result.unwrap() << ')';

        return os << ERR_PREFIX << result.unwrapError() << ')';
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Option<T>& option) {
        static constexpr std::string_view SOME_PREFIX = "Some(";
        static constexpr std::string_view NONE_STR = "None";

        if (option.some())
            return os << SOME_PREFIX << option.unwrap() << ')';

        return os << NONE_STR;
    }

}