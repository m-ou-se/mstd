#pragma once

#include <cstddef>
#include <type_traits>

namespace mstd {

template<typename T>
struct range {

	constexpr range() : begin_(nullptr), size_(0) {}

	constexpr range(T * b, std::size_t s) : begin_(b), size_(s) {}

	constexpr range(T * b, T * e) : begin_(b), size_(e - b) {}

	constexpr range(T & x) : begin_(&x), size_(1) {}

	template<std::size_t n>
	constexpr range(T (&x)[n]) : begin_(&x[0]), size_(n) {}

	template<
		typename X,
		typename = std::enable_if_t<
			std::is_convertible<decltype(std::declval<X>().data()), T *>::value &&
			std::is_convertible<decltype(std::declval<X>().size()), std::size_t>::value
		>
	>
	constexpr range(X & x) : begin_(x.data()), size_(x.size()) {}

	constexpr T & operator [] (std::size_t i) const { return begin_[i]; }

	constexpr T * begin() const { return begin_; }
	constexpr T *   end() const { return begin_ + size_; }

	constexpr void remove_prefix(std::size_t n) { begin_ += n; }
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
