#ifndef _CHERRY_ENCODE_BOOL_ENCODER_HPP_INCLUDED_
#define _CHERRY_ENCODE_BOOL_ENCODER_HPP_INCLUDED_

namespace cherry {

struct BoolEncoder {
	struct {
		uint32_t base;
		uint32_t length;
	} interval;
	unsigned char* output;
	int bitCount;

	BoolEncoder() {
		interval.base = 0;
		interval.length = 255;
		bitCount = 0;
	}

private:
	void propagateCarry(uint8_t* p) {
		while (*--p == 255) {
			*p = 0;
		}
		++*p;
	}

	void writeBool(uint8_t probability, bool value) {
		uint32_t split = 1 + (((range - 1) * probability) >> 8);
		if (value) {
			bottom += split;
			range -= split;
		} else {
			range = split;
		}
		while (range < 128) {
			range <<= 1;
			if (bottom & (1 << 31)) {
				propagateCarry(output);
			}
			bottom <<= 1;
			if (!--bitCount) {
				*output++ = (uint8_t)(bottom >> 24);
				bottom &= (1 << 24) - 1;
				bitCount = 8;
			}
		}
	}

	void flush() {
		int c = bitCount;
		uint32_t v = bottom;
		if (v & (1 << (32 - c))) {
			propagateCarry(output);
		}
		v <<= c & 7;
		c >>= 3;
		while (--c >= 0) {
			v <<= 8;
		}
		c = 4;
		while (--c >= 0) {
			*output++ = (uint8_t)(v >> 24);
			v <<= 8;
		}
	}
};

} // namespace cherry

#endif // _CHERRY_ENCODE_BOOL_ENCODER_HPP_INCLUDED_

