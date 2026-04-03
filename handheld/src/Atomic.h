#pragma once

#ifndef _HANDHELD_SRC_ATOMIC_H_6DE9AE8B_DA20_56C3_83B5_B4E5232B357D
#define _HANDHELD_SRC_ATOMIC_H_6DE9AE8B_DA20_56C3_83B5_B4E5232B357D

#if defined(__EPOC32__)

#include <e32atomics.h>
#include <cstddef>
#include <cstdint>

namespace detail {

template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };
}

template <class T>
struct Atomic {
	//static_assert(detail::is_integral<T>::value, "atomic must hold an integral");

	template <class TT, size_t M>
	using Measure = typename detail::enable_if<sizeof(TT) == M, TT>::type;

	Atomic(T v) : m_v(v) {}

	template <class TT = T>
	operator Measure<TT, 1>() const {
		return static_cast<T>(__e32_atomic_load_acq8(&m_v));
	}

	template <class TT = T>
	Measure<TT, 1> operator=(T v) {
		return __e32_atomic_swp_ord8(&m_v, v);
	}

	template <class TT = T>
	operator Measure<TT, 4>() const {
		return static_cast<T>(__e32_atomic_load_acq32(&m_v));
	}

	template <class TT = T>
	Measure<TT, 4> operator=(T v) {
		return __e32_atomic_swp_ord32(&m_v, v);
	}

private:
	T m_v;
};

#else

#include <atomic>

template <class T>
using Atomic = std::atomic<T>;

#endif

#endif

