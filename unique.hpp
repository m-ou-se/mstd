#pragma once

#include <utility>

namespace mstd {

// Like an unique_ptr, but without the 'ptr' part.
// Contains the value, instead of a pointer to a value.
// If bool(value) == true on destruction, Closer()(value) will called.
// A default constructed T should convert to false: bool(T()) == false.
template<typename T, typename Closer>
struct unique {

private:
	T value_;

public:
	unique() : value_() {}

	explicit unique(T value) : value_(std::move(value)) {}

	unique(unique && other) : value_(other.release()) {}

	~unique() { if (value_) Closer()(value_); }

	unique & operator = (unique && other) {
		if (value_) Closer()(value_);
		value_ = other.release();
	}

	T release() {
		T v = std::move(value_);
		value_ = T();
		return std::move(v);
	}

	explicit operator bool() const { return bool(value_); }

	T       &  get()       &  { return value_; }
	T const &  get() const &  { return value_; }
	T       && get()       && { return std::move(value_); }

	T       &  operator * ()       &  { return value_; }
	T const &  operator * () const &  { return value_; }
	T       && operator * ()       && { return std::move(value_); }

	T const * operator -> () const { return &value_; }
	T       * operator -> ()       { return &value_; }

	friend bool operator == (unique const & a, unique const & b) {
		return a.value_ == b.value_;
	}

	friend bool operator != (unique const & a, unique const & b) {
		return a.value_ != b.value_;
	}
};

}
