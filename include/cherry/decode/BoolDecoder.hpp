#ifndef _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_
#define _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_

#include <stdint.h>
#include <cherry/except/InvalidArgument.hpp>

namespace cherry {

class BoolDecoder {
public:
	BoolDecoder(const void* input, size_t size, uint8_t probability);

	unsigned char decode() {
		unsigned char result = 0;
		for (int i = 7; i >= 0; --i) {
			result *= 2;
			result += getBool();
		}
		return result;
	}

private:
	struct {
		const unsigned char* buffer;
		const unsigned char* cursor;
		size_t size;
	} stream;
	int shift;
	uint32_t length;
	uint32_t value;
	uint8_t probability;

	bool getBool();
};

} // namespace cherry

#endif // _CHERRY_DECODE_BOOL_DECODER_HPP_INCLUDED_
