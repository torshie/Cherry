#include <cherry/encode/BoolEncoder.hpp>

using namespace cherry;

void BoolEncoder::flush() {
	if (shift == 0) {
		return;
	}
	probability = 128;
	for (int i = 0; i < 32; ++i) {
		addBool(false);
	}
	shift = 0;
}

void BoolEncoder::propagateCarry() {
	unsigned char* cursor = stream.cursor - 1;
	while (*cursor == 0xff) {
		*cursor = 0;
		--cursor;
	}
	++(*cursor);
}
