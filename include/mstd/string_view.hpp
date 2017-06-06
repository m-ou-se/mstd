#pragma once
#if (__cplusplus <= 201402L || !__has_include(<string_view>)) && __has_include(<experimental/string_view>)
#include <experimental/string_view>
namespace mstd {
using std::experimental::string_view;
using std::experimental::basic_string_view;
}
#elif __has_include(<string_view>)
#include <string_view>
namespace mstd {
using std::string_view;
using std::basic_string_view;
}
#else
#error missing <string_view>
#endif
