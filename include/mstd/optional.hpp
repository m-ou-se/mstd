#pragma once
#if (__cplusplus <= 201402L || !__has_include(<optional>)) && __has_include(<experimental/optional>)
#include <experimental/optional>
namespace mstd {
using std::experimental::optional;
using std::experimental::nullopt;
using std::experimental::make_optional;
}
#elif __has_include(<optional>)
#include <optional>
namespace mstd {
using std::optional;
using std::nullopt;
using std::make_optional;
}
#else
#error Missing <optional>
#endif
