#ifndef _CHERRY_VP8_LOOP_FILTER_HPP_INCLUDED_
#define _CHERRY_VP8_LOOP_FILTER_HPP_INCLUDED_

#include <stdint.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/BlockInfo.hpp>
#include <cherry/vp8/LoopFilter.hpp>

namespace cherry {

class LoopFilter {
	struct Parameter {
		uint8_t threshold;
		struct {
			uint8_t edge;
			uint8_t subblock;
			uint8_t neighbor;
		} limit;

		void init(int level, uint8_t sharpness);
	};
public:
	LoopFilter() : sharpness(0xff) {
		std::memset(&delta, 0, sizeof(delta));
	}

	template<FilterType>
	void execute(Pixel* pixel, ptrdiff_t step, bool subblock);
	bool setLevel(const BlockInfo* info);
	void loadInfo(BoolDecoder* source);

	FilterType getType() const {
		return type;
	}

private:
	FilterType type;
	uint8_t level;
	uint8_t sharpness;
	struct {
		bool enabled;
		int8_t reference[4];
		int8_t mode[4];
	} delta;
	Parameter param[64];
	Parameter* current;

	static int8_t adjust(bool useOuter, const Pixel* P1,
			Pixel* P0, Pixel* Q0, const Pixel* Q1);
	static void edgeFilter(uint8_t threshold, Pixel** pixel);
	static void subblockFilter(uint8_t threshold, Pixel** pixel);
	static bool suitable(uint8_t neighbor, uint8_t edge, Pixel** pixel);

	static int8_t clamp(int value) {
		if (value < -128) {
			return -128;
		} else if (value > 127) {
			return 127;
		} else {
			return value;
		}
	}

	static bool highEdgeVariance(uint8_t threshold, int8_t p1,
			int8_t p0, int8_t q0, int8_t q1) {
		return std::abs(p1 - p0) > threshold
				|| std::abs(q1 - q0) > threshold;
	}
};

} // namespace cherry

#endif // _CHERRY_VP8_LOOP_FILTER_HPP_INCLUDED_
