#ifndef _CHERRY_VP8_BLOCK_INFO_HPP_INCLUDED_
#define _CHERRY_VP8_BLOCK_INFO_HPP_INCLUDED_

#include <cstring>
#include <cherry/vp8/const.hpp>
#include <cherry/utility/misc.hpp>

namespace cherry {

struct BlockInfo {
	int8_t submode[16];
	int8_t lastCoeff[25];
	int8_t lumaMode;
	int8_t chromaMode;
	int8_t segment;
	bool skipCoeff;

	BlockInfo() {
		std::memset(lastCoeff, 0, sizeof(lastCoeff));
	}

	bool hasVirtualPlane() const {
		return lumaMode != kDummyLumaMode;
	}

	void initForPadding() {
		lumaMode = kAverageMode;
		chromaMode = kAverageMode;
		for (size_t i = 0; i < ELEMENT_COUNT(submode); ++i) {
			submode[i] = kAverageMode;
		}
	}

	void setDummySubmode() {
		const static int8_t kMap[] = {
			kAverageSubmode, kVerticalSubmode, kHorizontalSubmode,
			kTrueMotionSubmode
		};
		std::memset(submode, kMap[lumaMode], sizeof(submode));
	}
};

} // namespace cherry

#endif // _CHERRY_VP8_BLOCK_INFO_HPP_INCLUDED_
