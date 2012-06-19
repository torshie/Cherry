#ifndef _CHERRY_UTILITY_BOOL_STRING_HPP_INCLUDED_
#define _CHERRY_UTILITY_BOOL_STRING_HPP_INCLUDED_

#include <stdint.h>
#include <cstddef>
#include <cherry/utility/integer.hpp>
#include <cherry/utility/bit.hpp>

namespace cherry {

class BoolString {
public:
	BoolString(void* buffer, size_t size)
			: buffer((unsigned char*)buffer),
			cursor((unsigned char*)buffer), size(size), shift(0) {
	}

	template<int BIT_COUNT>
	uint8_t read() {
		typedef char _guard[BIT_COUNT >= 1 && BIT_COUNT <= 8 ? 1 : -1];
		if (BIT_COUNT == 8 && shift == 0) {
			return *cursor++;
		}
		if (BIT_COUNT + shift <= 8) {
			uint8_t value = (*cursor & ((1 << BIT_COUNT) - 1) << shift)
					>> shift;
			shift += BIT_COUNT;
			if (shift == 8) {
				shift = 0;
				++cursor;
			}
			return value;
		}
		int lower = 8 - shift;
		int higher = BIT_COUNT - lower;
		uint8_t value = (*cursor & (((1 << lower) - 1) << shift)) >> shift;
		++cursor;
		value |= (*cursor & ((1 << higher) - 1)) << lower;
		shift = higher;
		return value;
	}

private:
	unsigned char* buffer;
	unsigned char* cursor;
	size_t size;
	int shift;
};

} // namespace cherry

#endif // _CHERRY_UTILITY_BOOL_STRING_HPP_INCLUDED_
