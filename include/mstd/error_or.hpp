#pragma once

#include <new>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>

namespace mstd {

// error_or<T> represents either a T or an error.
//
// T can be anything that's movable, a reference, or void.
//
// Example:
//   mstd::error_or<int> test() {
//     if (!foo()) {
//       return make_error_code(std::errc::io_error);
//     } else {
//       return 7;
//     }
//   }
//
//   void f() {
//     if (mstd::error_or<int> result = test()) {
//       std::cout << result.value();
//     } else {
//       std::cerr << "Error: " << result.error();
//     }
//   }
//
// By default, std::error_code is used for errors, but an anternative error
// type can be given as the second template argument: error_or<T, Error>
//
// Error must be able to represent 'no error', and must value-initialize to a
// 'no error' value.
//
// Normally, Error must also be explicitly convertible to bool: bool(err)
// should evaluate to false if and only if err is set to a 'no error' value.
// However, a different method to check if an error contains a 'no error' value
// can be given with a third template argument: error_or<T, Error, ErrorIsOk>.
// In that case, ErrorIsOk()(e) must evaluate to true if and only if e is a 'no
// error' value.
//
// This means enums, enum classes, integers, std::error_code and even
// std::unique_ptr are all usable as Error type with the default ErrorIsOk.
// (Enums and enum classes do not explicitly need a name for the 0 value, but
// can't use 0 for anything other than 'no error'.)
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
// value, since in that case, there should've been a value. The specialization
// for T=void doesn't have this restriction.
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

namespace detail_ {

struct error_is_ok {
	template<typename E>
	bool operator () (E const & e) {
		return !bool(e);
	}
};

}

template<
	typename T,
	typename Error = std::error_code,
	typename ErrorIsOk = detail_::error_is_ok
>
class error_or {

	Error error_;

	struct Value { T value_; };
	union { Value value_; };

public:

	// Implicit conversions from both Error and T.

	error_or(Error e) : error_(std::move(e)) {
		// An error_or<T> without an error needs a value.
		if (ok()) throw std::invalid_argument("error_or(Error)");
	}

	error_or(T value) : error_() {
		new (&value_) Value{static_cast<T &&>(value)};
	}

	// Move and copy constructors.

	error_or(error_or && other) noexcept(
		noexcept(Error(std::move(other.error_))) &&
		noexcept(Value{static_cast<T &&>(other.value_.value_)})
	) : error_(std::move(other.error_)) {
		if (ok()) new (&value_) Value{static_cast<T &&>(other.value_.value_)};
	}

	error_or(error_or const & other) : error_(other.error_) {
		if (ok()) new (&value_) Value{other.value_};
	}

	// Move and copy assignment.

	error_or & operator = (error_or && other) noexcept(
		noexcept(value_.value_ = std::move(other.value_.value_)) &&
		noexcept(Value{static_cast<T &&>(other.value_.value_)}) &&
		noexcept(error_ = std::move(other.error_))
	) {
		if (ok() && other.ok()) {
			value_.value_ = std::move(other.value_.value_);
		} else if (ok()) {
			value_.~Value();
		} else if (other.ok()) {
			new (&value_) Value{static_cast<T &&>(other.value_.value_)};
		}
		error_ = std::move(other.error_);
		return *this;
	}

	error_or & operator = (error_or const & other) {
		if (ok() && other.ok()) {
			value_.value_ = other.value_.value_;
		} else if (ok()) {
			value_.~Value();
		} else if (other.ok()) {
			new (&value_) Value{other.value_.value_};
		}
		error_ = other.error_;
		return *this;
	}

	// Destructor.

	~error_or() {
		if (ok()) value_.~Value();
	}

	// Explicit accessors.

	bool ok() const { return ErrorIsOk()(error_); }

	Error       &  error()       &  { return error_; }
	Error const &  error() const &  { return error_; }
	Error       && error()       && { return std::move(error_); }

	T       &  value()       &  { return value_.value_; }
	T const &  value() const &  { return value_.value_; }
	T       && value()       && { return static_cast<T &&>(value_.value_); }

	template<typename U>
	T value_or(U && default_value) const & {
		return ok() ? value_.value_ : static_cast<T>(std::forward<U>(default_value));
	}

	template<typename U>
	T value_or(U && default_value) && {
		return ok() ? static_cast<T &&>(value_.value_) : static_cast<T>(std::forward<U>(default_value));
	}

	// Implicit accessors.

	explicit operator bool() const { return ok(); }

	T       &  operator * ()       &  { return value_.value_; }
	T const &  operator * () const &  { return value_.value_; }
	T       && operator * ()       && { return static_cast<T &&>(value_.value_); }

	typename std::remove_reference<T>::type       * operator -> ()       { return &value_.value_; }
	typename std::remove_reference<T>::type const * operator -> () const { return &value_.value_; }

};

template<typename Error, typename ErrorIsOk>
class error_or<void, Error, ErrorIsOk> {

	Error error_;

public:
	error_or(Error e) : error_(std::move(e)) {}

	error_or() : error_() {}

	bool ok() const { return ErrorIsOk()(error_); }

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
	if (!a.ok()) return false;
	return a.value() == b;
}

template<typename T, typename Error>
inline bool operator != (error_or<T, Error> const & a, T const & b) {
	if (!a.ok()) return true;
	return a.value() != b;
}

template<typename T, typename Error>
inline bool operator == (T const & a, error_or<T, Error> const & b) {
	if (!b.ok()) return false;
	return a == b.value();
}

template<typename T, typename Error>
inline bool operator != (T const & a, error_or<T, Error> const & b) {
	if (!b.ok()) return true;
	return a != b.value();
}

// Comparison between error_ors.

template<typename T1, typename E1, typename T2, typename E2>
inline bool operator == (error_or<T1, E1> const & a, error_or<T2, E2> const & b) {
	if (a.ok() && b.ok()) return a.value() == b.value();
	return a.error() == b.error();
}

template<typename T1, typename E1, typename T2, typename E2>
inline bool operator != (error_or<T1, E1> const & a, error_or<T2, E2> const & b) {
	if (a.ok() && b.ok()) return a.value() != b.value();
	return a.error() != b.error();
}

template<typename E1, typename E2>
inline bool operator == (error_or<void, E1> const & a, error_or<void, E2> const & b) {
	if (a.ok() && b.ok()) return true;
	return a.error() == b.error();
}

template<typename E1, typename E2>
inline bool operator != (error_or<void, E1> const & a, error_or<void, E2> const & b) {
	if (a.ok() && b.ok()) return false;
	return a.error() != b.error();
}

}
