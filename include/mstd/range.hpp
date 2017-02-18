#pragma once

#include <cstddef>
#include <initializer_list>
#include <type_traits>

namespace mstd {

namespace range_detail {

template<typename T>
auto data(T & v) -> decltype(v.data()) { return v.data(); }

template<typename T>
T const * data(std::initializer_list<T> v) { return v.begin(); }

template<typename T, size_t N>
T * data(T (&x)[N]) { return x; }

template<typename T>
auto size(T & v) -> decltype(v.size()) { return v.size(); }

template<typename T, size_t N>
size_t size(T (&x)[N]) { return N; }

}

template<typename T>
struct range {

	constexpr range() : begin_(nullptr), size_(0) {}

	constexpr range(decltype(nullptr)) : begin_(nullptr), size_(0) {}

	constexpr range(T * b, std::size_t s) : begin_(b), size_(s) {}

	constexpr range(T * b, T * e) : begin_(b), size_(e - b) {}

	constexpr range(T & x) : begin_(&x), size_(1) {}

	template<
		typename X,
		typename = std::enable_if_t<
			std::is_convertible<decltype(range_detail::data(std::declval<X &>())), T *>::value &&
			std::is_convertible<decltype(range_detail::size(std::declval<X &>())), std::size_t>::value
		>
	>
	constexpr range(X & x) : begin_(range_detail::data(x)), size_(range_detail::size(x)) {}

	constexpr T & operator [] (std::size_t i) const { return begin_[i]; }

	constexpr T * begin() const { return begin_; }
	constexpr T *   end() const { return begin_ + size_; }

	constexpr void remove_prefix(std::size_t n) { begin_ += n; size_ -= n; }
	constexpr void remove_suffix(std::size_t n) { size_ -= n; }

	constexpr std::size_t size() const { return size_; }

	constexpr T * data() const { return begin_; }

	constexpr bool empty() const { return size_ == 0; }

	constexpr range subrange(std::size_t pos, std::size_t count = std::size_t(-1)) {
		return range(begin_ + pos, size_ - pos < count ? size_ - pos : count);
	}

	friend constexpr bool operator == (range a, range b) {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i) if (!(a[i] == b[i])) return false;
		return true;
	}

	friend constexpr bool operator != (range a, range b) {
		if (a.size() != b.size()) return true;
		for (size_t i = 0; i < a.size(); ++i) if (a[i] != b[i]) return true;
		return false;
	}

private:
	T * begin_;
	std::size_t size_;
};

}
