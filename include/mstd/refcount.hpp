#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

namespace mstd {

class refcounted {
	mutable std::atomic<std::size_t> references{0};

public:
	refcounted() noexcept {}

	refcounted(refcounted const &) noexcept {}
	refcounted(refcounted &&) noexcept {}

	refcounted & operator=(refcounted const &) noexcept { return *this; }
	refcounted & operator=(refcounted &&) noexcept { return *this; }

	friend std::size_t use_count(refcounted const * object) noexcept {
		return object->references;
	}

	friend std::size_t unique(refcounted const * object) noexcept {
		return object->references == 1;
	}

	friend std::size_t increment_refcount(refcounted const * object) noexcept {
		return ++object->references;
	}

	template<typename T>
	friend std::size_t decrement_refcount(T const * object) noexcept {
		auto n_refs = --static_cast<refcounted const *>(object)->references;
		if (!n_refs) delete object;
		return n_refs;
	}
};

template<typename T> class refcount_ptr;
template<typename T, typename... Args> refcount_ptr<T> make_refcount(Args &&...);

template<typename T>
class refcount_wrapper final : public refcounted {
	T wrapped;

	template<typename... Args>
	explicit refcount_wrapper(Args &&... args)
		: wrapped(std::forward<Args>(args)...) {}

	template<typename>
	friend class refcount_ptr;

	template<typename T2, typename... Args>
	friend refcount_ptr<T2> make_refcount(Args &&...);
};

template<typename T>
class refcount_ptr {

public:
	using element_type = T;

	using refcounted_type = typename std::conditional<
		std::is_convertible<T *, refcounted const *>::value,
		T,
		refcount_wrapper<T>
	>::type;

private:
	refcounted_type * object;

	template<typename>
	friend class refcount_ptr;

	template<typename T2, typename... Args>
	friend refcount_ptr<T2> make_refcount(Args &&...);

public:
	refcount_ptr(std::nullptr_t = nullptr) noexcept : object(nullptr) {}

	refcount_ptr(refcounted_type * object) noexcept : object(object) {
		if (object) increment_refcount(object);
	}

	refcount_ptr(refcount_ptr const & other) noexcept : object(other.object) {
		if (object) increment_refcount(object);
	}

	refcount_ptr(refcount_ptr && other) noexcept : object(other.object) {
		other.object = nullptr;
	}

	template<
		typename T2,
		typename = typename std::enable_if<std::is_convertible<
			typename refcount_ptr<T2>::refcounted_type *,
			refcounted_type *
		>::value>::type
	>
	refcount_ptr(refcount_ptr<T2> const & other) noexcept : object(other.object) {
		if (object) increment_refcount(object);
	}

	template<
		typename T2,
		typename = typename std::enable_if<std::is_convertible<
			typename refcount_ptr<T2>::refcounted_type *,
			refcounted_type *
		>::value>::type
	>
	refcount_ptr(refcount_ptr<T2> && other) noexcept : object(other.object) {
		other.object = nullptr;
	}

	refcount_ptr & operator=(refcounted_type * other) noexcept {
		if (other != object) {
			if (object) decrement_refcount(object);
			object = other;
			if (object) increment_refcount(object);
		}
		return *this;
	}

	refcount_ptr & operator=(refcount_ptr const & other) noexcept {
		return *this = other.object;
	}

	refcount_ptr & operator=(refcount_ptr && other) noexcept {
		if (&other != this) {
			if (object) decrement_refcount(object);
			object = other.object;
			other.object = nullptr;
		}
		return *this;
	}

	template<
		typename T2,
		typename = typename std::enable_if<std::is_convertible<
			typename refcount_ptr<T2>::refcounted_type *,
			refcounted_type *
		>::value>::type
	>
	refcount_ptr & operator=(refcount_ptr<T2> const & other) noexcept {
		return *this = other.object;
	}

	template<
		typename T2,
		typename = typename std::enable_if<std::is_convertible<
			typename refcount_ptr<T2>::refcounted_type *,
			refcounted_type *
		>::value>::type
	>
	refcount_ptr & operator=(refcount_ptr<T2> && other) noexcept {
		if (object) decrement_refcount(object);
		object = other.object;
		other.object = nullptr;
		return *this;
	}

	~refcount_ptr() noexcept {
		if (object) decrement_refcount(object);
	}

	T * get() const noexcept {
		return object ? unwrap(object) : nullptr;
	}

	explicit operator bool () const noexcept {
		return object != nullptr;
	}

	T & operator*() const noexcept {
		return *unwrap(object);
	}

	T * operator->() const noexcept {
		return unwrap(object);
	}

	std::size_t use_count() const noexcept {
		return object ? use_count(object) : 0;
	}

	bool unique() const noexcept {
		return use_count() == 1;
	}

	template<typename T2, typename T1>
	friend refcount_ptr<T2> static_pointer_cast(refcount_ptr<T1> p);

	template<typename T2, typename T1>
	friend refcount_ptr<T2> dynamic_pointer_cast(refcount_ptr<T1> const & p);

	template<typename T2, typename T1>
	friend refcount_ptr<T2> dynamic_pointer_cast(refcount_ptr<T1> && p);

	template<typename T2, typename T1>
	friend refcount_ptr<T2> const_pointer_cast(refcount_ptr<T1> p);

private:
	static T * unwrap(T * x) noexcept { return x; }
	static T * unwrap(refcount_wrapper<T> * x) noexcept { return &x->wrapped; }
};

template<typename T2, typename T>
refcount_ptr<T2> static_pointer_cast(refcount_ptr<T> p) {
	refcount_ptr<T2> p2;
	p2.object = static_cast<typename refcount_ptr<T2>::refcounted_type *>(p.object);
	p.object = nullptr;
	return std::move(p2);
}

template<typename T2, typename T>
refcount_ptr<T2> dynamic_pointer_cast(refcount_ptr<T> const & p) {
	return refcount_ptr<T2>(dynamic_cast<typename refcount_ptr<T2>::refcounted_type *>(p.object));
}

template<typename T2, typename T>
refcount_ptr<T2> dynamic_pointer_cast(refcount_ptr<T> && p) {
	refcount_ptr<T2> p2;
	p2.object = dynamic_cast<typename refcount_ptr<T2>::refcounted_type *>(p.object);
	if (p2.object) p.object = nullptr;
	return std::move(p2);
}

template<typename T2, typename T>
refcount_ptr<T2> const_pointer_cast(refcount_ptr<T> p) {
	refcount_ptr<T2> p2;
	p2.object = const_cast<typename refcount_ptr<T2>::refcounted_type *>(p.object);
	p.object = nullptr;
	return std::move(p2);
}

template<typename A, typename B>
bool operator==(refcount_ptr<A> const & a, refcount_ptr<B> const & b) {
	return a.get() == b.get();
}

template<typename A, typename B>
bool operator!=(refcount_ptr<A> const & a, refcount_ptr<B> const & b) {
	return a.get() != b.get();
}

template<typename T>
bool operator==(std::nullptr_t, refcount_ptr<T> const & p) {
	return !bool(p);
}

template<typename T>
bool operator==(refcount_ptr<T> const & p, std::nullptr_t) {
	return !bool(p);
}

template<typename T>
bool operator!=(std::nullptr_t, refcount_ptr<T> const & p) {
	return bool(p);
}

template<typename T>
bool operator!=(refcount_ptr<T> const & p, std::nullptr_t) {
	return bool(p);
}

template<typename T, typename... Args>
refcount_ptr<T> make_refcount(Args &&... args) {
	return new typename refcount_ptr<T>::refcounted_type(std::forward<Args>(args)...);
}

}
