#ifndef _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_
#define _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_

#include <stdint.h>
#include <cstddef>
#include <cherry/vp8/types.hpp>
#include <cherry/utility/integer.hpp>

namespace cherry {

class BoolDecoder {
public:
	BoolDecoder(const void* input, size_t size);

	bool decode(Probability probability);
	int8_t decode(const TreeIndex* tree, const Probability* table);

	template<int WIDTH>
	typename U<WIDTH>::T uint() {
		typedef char _guard[WIDTH >= 1 && WIDTH <= 64 ? 1 : -1];
		typename U<WIDTH>::T result = 0;
		for (int i = 0; i < WIDTH; ++i) {
			result *= 2;
			result += decode(128);
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
