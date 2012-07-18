#ifndef _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_
#define _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_

#include <stdint.h>
#include <cstddef>
#include <cherry/vp8/types.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/utility/integer.hpp>

namespace cherry {

class BoolDecoder {
public:
	void reload(const void* input, size_t size);
	IntraMode decodeLumaMode();
	IntraMode decodeChromaMode();
	SubblockMode decodeSubblockMode(const Probability* probability);
	int_fast8_t decodeSegmentId(const Probability* probability);

	bool decode(Probability probability) {
		uint32_t split = 1 + (length - 1) * probability / 256;
		bool result = false;
		if (value >= split * 256) {
			result = true;
			length -= split;
			value -= split * 256;
		} else {
			length = split;
		}

		while (length < 128) {
			value *= 2;
			length *= 2;
			++shift;
			if (shift == 8) {
				shift = 0;
				if (cursor - buffer < (ptrdiff_t)size) {
					value |= *cursor;
					++cursor;
				}
			}
		}

		return result;
	}

	template<int WIDTH>
	typename U<WIDTH>::T uint() {
		typedef char _guard[WIDTH >= 1 && WIDTH <= 64 ? 1 : -1];
		typename U<WIDTH>::T result = 0;
		for (int i = 0; i < WIDTH; ++i) {
			result += result + decode(128);
		}
		return result;
	}

	template<int WIDTH>
	typename I<WIDTH>::T sint() {
		typedef char _guard[WIDTH >= 2 && WIDTH <= 64 ? 1 : -1];
		typename I<WIDTH>::T result = uint<WIDTH - 1>();
		if (decode(128)) {
			return -result;
		} else {
			return result;
		}
	}

private:
	int shift;
	uint32_t length;
	uint32_t value;
	const uint8_t* buffer;
	const uint8_t* cursor;
	size_t size;
};

} // namespace cherry

#endif // _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_
