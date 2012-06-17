#ifndef _CHERRY_WRAPPER_WRAPPER_HPP_INCLUDED_
#define _CHERRY_WRAPPER_WRAPPER_HPP_INCLUDED_

#include <cherry/except/SystemError.hpp>

#define CHECK_RETURN_ZERO(...) \
	do { \
		if ((__VA_ARGS__) != 0) { \
			RAISE(SystemError, errno); \
		} \
	} while (false)

#define CHECK_NON_NEGATIVE(type, ...) \
	type value = (__VA_ARGS__); \
	if (value < 0) { \
		RAISE(SystemError, errno); \
	} \
	return value

#include <cherry/wrapper/libc.hpp>
#include <cherry/wrapper/unix.hpp>

#undef CHECK_RETURN_ZERO
#undef CHECK_NON_NEGATIVE

#endif // _CHERRY_WRAPPER_WRAPPER_HPP_INCLUDED_
