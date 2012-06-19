#ifndef _CHERRY_UTILITY_INTEGER_HPP_INCLUDED_
#define _CHERRY_UTILITY_INTEGER_HPP_INCLUDED_

#include <stdint.h>

namespace cherry {

template<int WIDTH> struct U {
	typedef typename U<WIDTH + 1>::T T;
};

template<int WIDTH> struct I {
	typedef typename I<WIDTH + 1>::T T;
};

template<> struct U<8> {
	typedef uint8_t T;
};

template<> struct U<16> {
	typedef uint16_t T;
};

template<> struct U<32> {
	typedef uint32_t T;
};

template<> struct U<64> {
	typedef uint64_t T;
};

template<> struct I<8> {
	typedef int8_t T;
};

template<> struct I<16> {
	typedef int16_t T;
};

template<> struct I<32> {
	typedef int32_t T;
};

template<> struct I<64> {
	typedef int64_t T;
};

} // namespace cherry

#endif // _CHERRY_UTILITY_INTEGER_HPP_INCLUDED_
