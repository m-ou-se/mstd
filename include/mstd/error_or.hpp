#pragma once

#include <new>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace mstd {

// error_or<T> represents either a T or an error.
//
// T can be anything that's movable, or void.
//
// By default, std::error_code is used for errors, but an anternative error
// type can be given as the second template argument:
//   error_or<T, Error>
//
// Error must be able to represent 'no error', and must value-initialize to a
// 'no error' value. It must also be explicitly convertible to bool: With Error
// err; bool(err) should evaluate to false if it is set to a 'no error' value,
// or true otherwise.
//
// This means enums, enum classes, integers, and std::error_code are all usable
// as Error type. Enums and enum classes do not explicitly need a 0 value, but
// can't use 0 for an error value.
//
// Example:
//   enum class ErrorCode {
//     timeout = 1,
//     foobar = 2
//   };
//   mstd::error_or<std::string, ErrorCode> get_something();
//
// error_or<T, Error> can be implicitly constructed from either a T or an
// Error. The only restriction is that the Error must not be a 'no error'
// value, since in that case, there should've been a value (a T). The
// specialization for T=void doesn't have this restriction.
//
// Example:
//   mstd::error_or<std::string, ErrorCode> get_something() {
//     if (foobar) {
//       return ErrorCode::foobar;
//     }
//     if (auto result = get_something_else()) {
//       return result.value() + "!";
//     } else {
//       return result.error();
//     }
//   }
template<typename T, typename Error = std::error_code>
class error_or {

	Error error_;
	union { T value_; };

public:

	// Implicit conversions from both Error and T.

	error_or(Error e) : error_(std::move(e)) {
		// An error_or<T> without an error needs a value.
		if (ok()) throw std::invalid_argument("error_or(Error)");
	}

	error_or(T value) : error_() {
		new (&value_) T(std::move(value));
	}

	// Move and copy constructors.

	error_or(error_or && other) noexcept(
		noexcept(Error(std::move(other.error_))) &&
		noexcept(T(std::move(other.value_)))
	) : error_(std::move(other.error_)) {
		if (ok()) new (&value_) T(std::move(other.value_));
	}

	error_or(error_or const & other) : error_(other.error_) {
		if (ok()) new (&value_) T(other.value_);
	}

	// Move and copy assignment.

	error_or & operator = (error_or && other) noexcept(
		noexcept(value_ = std::move(other.value_)) &&
		noexcept(value_.~T()) &&
		noexcept(T(std::move(other.value_))) &&
		noexcept(error_ = std::move(other.error_))
	) {
		if (ok() && other.ok()) {
			value_ = std::move(other.value_);
		} else if (ok()) {
			value_.~T();
		} else if (other.ok()) {
			new (&value_) T(std::move(other.value_));
		}
		error_ = std::move(other.error_);
		return *this;
	}

	error_or & operator = (error_or const & other) {
		if (ok() && other.ok()) {
			value_ = other.value_;
		} else if (ok()) {
			value_.~T();
		} else if (other.ok()) {
			new (&value_) T(other.value_);
		}
		error_ = other.error_;
		return *this;
	}

	// Destructor.

	~error_or() noexcept(noexcept(value_.~T())) {
		if (ok()) value_.~T();
	}

	// Explicit accessors.

	bool ok() const { return !bool(error_); }

	Error       &  error()       &  { return error_; }
	Error const &  error() const &  { return error_; }
	Error       && error()       && { return std::move(error_); }

	T       &  value()       &  { return value_; }
	T const &  value() const &  { return value_; }
	T       && value()       && { return std::move(value_); }

	// Implicit accessors.

	explicit operator bool() const { return ok(); }

	T       &  operator * ()       &  { return value_; }
	T const &  operator * () const &  { return value_; }
	T       && operator * ()       && { return std::move(value_); }

	T       * operator -> ()       { return &value_; }
	T const * operator -> () const { return &value_; }

};

template<typename Error>
class error_or<void, Error> {

	Error error_;

public:
	error_or(Error e) : error_(std::move(e)) {}

	error_or() : error_() {}

	bool ok() const { return !bool(error_); }

	explicit operator bool() const { return ok(); }

	Error       &  error()       &  { return error_; }
	Error const &  error() const &  { return error_; }
	Error       && error()       && { return std::move(error_); }

};

// Comparison between error_or and error.

template<typename T, typename Error>
inline bool operator == (error_or<T, Error> const & a, Error const & b) {
	return a.error() == b;
}

template<typename T, typename Error>
inline bool operator != (error_or<T, Error> const & a, Error const & b) {
	return a.error() != b;
}

template<typename T, typename Error>
inline bool operator == (Error const & a, error_or<T, Error> const & b) {
	return a == b.error();
}

template<typename T, typename Error>
inline bool operator != (Error const & a, error_or<T, Error> const & b) {
	return a != b.error();
}

// Comparison between error_or and value.

template<typename T, typename Error>
inline bool operator == (error_or<T, Error> const & a, T const & b) {
	if (a.error()) return false;
	return a.value() == b;
}

template<typename T, typename Error>
inline bool operator != (error_or<T, Error> const & a, T const & b) {
	if (a.error()) return true;
	return a.value() != b;
}

template<typename T, typename Error>
inline bool operator == (T const & a, error_or<T, Error> const & b) {
	if (b.error()) return false;
	return a == b.value();
}

template<typename T, typename Error>
inline bool operator != (T const & a, error_or<T, Error> const & b) {
	if (b.error()) return true;
	return a != b.value();
}

// Comparison between error_ors.

template<typename T1, typename E1, typename T2, typename E2>
inline bool operator == (error_or<T1, E1> const & a, error_or<T2, E2> const & b) {
	if (a.error() != b.error()) return false;
	if (a.error()) return true;
	return a.value() == b.value();
}

template<typename T1, typename E1, typename T2, typename E2>
inline bool operator != (error_or<T1, E1> const & a, error_or<T2, E2> const & b) {
	if (a.error() != b.error()) return true;
	if (a.error()) return false;
	return a.value() != b.value();
}

template<typename E1, typename E2>
inline bool operator == (error_or<void, E1> const & a, error_or<void, E2> const & b) {
	return a.error() == b.error();
}

template<typename E1, typename E2>
inline bool operator != (error_or<void, E1> const & a, error_or<void, E2> const & b) {
	return a.error() != b.error();
}

}
