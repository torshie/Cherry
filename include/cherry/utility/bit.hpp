#ifndef _CHERRY_UTILITY_BIT_HPP_INCLUDED_
#define _CHERRY_UTILITY_BIT_HPP_INCLUDED_

#include <stdint.h>

namespace cherry {

inline int getHammingWeight(uint32_t i) {
	// You are not meant to understand or maintain this code, just worship
	// the gods that revealed it to mankind.
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
}

template<typename Integer>
inline Integer reverseByteOrder(Integer value) {
	// A good compiler should be able to optimize this function into some
	// kind of bit-swap instruction if supported by the underlying
	// processor.
	switch (sizeof(Integer)) {
	case 8: {
		uint64_t tmp = value;
		tmp = ((tmp & 0x00000000ffffffffull) << 32)
				| (tmp >> 32);
		tmp = ((tmp & 0x0000ffff0000ffffull) << 16)
				| ((tmp & 0xffff0000ffff0000ull) >> 16);
		return ((tmp & 0x00ff00ff00ff00ffull) << 8)
				| ((tmp & 0xff00ff00ff00ff00ull) >> 8); }
	case 4: {
		uint32_t tmp = value;
		tmp = ((tmp & 0x0000ffff) << 16) | (tmp >> 16);
		return ((tmp & 0x00ff00ff) << 8) | ((tmp & 0xff00ff00) >> 8); }
	case 2: {
		uint16_t tmp = value;
		tmp = ((tmp & 0x00ff) << 8) | ((tmp & 0xff00) >> 8);
		return tmp; }
	default:
		return value;
	}
}

template<typename Integer>
inline Integer decodeLittleEndian(Integer value) {
	// The performance should be optimal under a good compiler.
	int needle = 1;
	if (*(char*)&needle == 1) {
		return value;
	} else {
		return reverseByteOrder(value);
	}
}

} // namespace cherry

#endif // _CHERRY_UTILITY_BIT_HPP_INCLUDED_
