#pragma once

// if someone tells me i can not use rust, i will make my own rust. period.
#define Try(expr) ({ auto __tmp_expect = (expr); if (!__tmp_expect) return std::unexpected(__tmp_expect.error()); *__tmp_expect; })
#define Propagate(expr) ({ if (const auto __tmp_err = (expr); __tmp_err.has_value()) return std::unexpected(*__tmp_err); })