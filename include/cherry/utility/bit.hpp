#ifndef _CHERRY_UTILITY_BIT_HPP_INCLUDED_
#define _CHERRY_UTILITY_BIT_HPP_INCLUDED_

#include <stdint.h>

namespace cherry {

int getHammingWeight(int32_t i) {
	// You are not meant to understand or maintain this code, just worship
	// the gods that revealed it to mankind.
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

} // namespace cherry

#endif // _CHERRY_UTILITY_BIT_HPP_INCLUDED_
