#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/wrapper/wrapper.hpp>

using namespace cherry;

BoolDecoder::BoolDecoder(const void* input, size_t size)
		: shift(0), length(255), buffer((const uint8_t*)input),
		cursor(buffer + 2), size(size) {
	if (size <= 2) {
		RAISE(FeatureIncomplete, "String fed to BoolDecoder is too short");
	}
	value = ((uint32_t)buffer[0] << 8) + buffer[1];
}

bool BoolDecoder::decode(Probability probability) {
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

#if 0
	static int sequence = 0;
	static FILE* file = NULL;
	++sequence;
	if (file == NULL) {
		file = wrapper::fopen_("/home/witch/cherry-log.log", "w");
	}
	std::fprintf(file, "%d %d %d\n", result, probability, sequence);
	wrapper::fflush_(file);
#endif

	return result;
}

int8_t BoolDecoder::decode(const TreeIndex* tree,
		const Probability* table) {
	TreeIndex index = 0;
	while ((index = tree[index + decode(table[index / 2])]) > 0) {
		// Empty
	}
	return -index;
}
