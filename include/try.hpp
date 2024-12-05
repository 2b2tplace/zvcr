#pragma once

template<typename E>
struct Error {
    E error;

    explicit Error(E&& error) : error(std::forward<E>(error)) {}
    Error(const Error&) = default;
    Error(Error&&) = default;
    Error(const E&) : error(std::forward<E>(error)) {}

    const E& get() const {
        return error;
    }
};

// if someone tells me i can not use rust, i will make my own rust. period.
template <typename T, typename E>
class Result {
    std::variant<T, Error<E>> valueOrError;
public:
    Result(T&& value) : valueOrError(std::variant<T, Error<E>>{value}) {}
    Result(const Error<E>& error) : valueOrError(std::variant<T, Error<E>>{error}) {}

    bool ok() const {
        return std::holds_alternative<T>(valueOrError);
    }

    const T& value() const {
        if (!ok()) throw std::runtime_error("Trying to access non-existent value in Result<T, E>");
        return std::get<T>(valueOrError);
    }

    const E& error() const {
        if (ok()) throw std::runtime_error("Trying to access non-existent error in Result<T, E>");
        return std::get<Error<E>>(valueOrError).get();
    }
};

#define Try(expr) ({ auto __tmp_expect = (expr); if (!__tmp_expect.ok()) return Error(__tmp_expect.error()); __tmp_expect.value(); })
#define Propagate(expr) ({ if (const auto __tmp_err = (expr); __tmp_err.has_value()) return Error(__tmp_err.value()); })