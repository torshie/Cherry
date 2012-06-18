#include <cherry/decode/BoolDecoder.hpp>

using namespace cherry;

BoolDecoder::BoolDecoder(const void* input, size_t size, uint8_t probability)
		: shift(0), length(255), probability(probability) {
	stream.buffer = static_cast<const unsigned char*>(input);
	stream.size = size;
	if (size >= 2) {
		stream.cursor = stream.buffer + 2;
		value = (uint32_t)stream.buffer[0] * 256 + stream.buffer[1];
	} else if (size == 0) {
		RAISE(InvalidArgument, "Parameter *size* must be positive");
	} else {
		stream.cursor = stream.buffer + 1;
		value = stream.buffer[0];
	}
}

bool BoolDecoder::getBool() {
	uint32_t split = 1 + (length - 1) * probability / 256;
	uint32_t valueSplit = split * 256;
	bool result = false;
	if (value >= valueSplit) {
		result = true;
		length -= split;
		value -= valueSplit;
	} else {
		length = split;
	}

	while (length < 128) {
		value *= 2;
		length *= 2;
		++shift;
		if (shift == 8) {
			shift = 0;
			if (stream.cursor - stream.buffer < (ptrdiff_t)stream.size) {
				value |= *stream.cursor;
				++stream.cursor;
			}
		}
	}

	return result;
}
