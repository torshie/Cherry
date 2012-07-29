#ifndef _CHERRY_VP8_CONST_HPP_INCLUDED_
#define _CHERRY_VP8_CONST_HPP_INCLUDED_

#include <cherry/vp8/types.hpp>

namespace cherry {

enum FilterType {
	kNormalFilter, kSimpleFilter
};

enum IntraMode {
	kAverageMode, kVerticalMode, kHorizontalMode, kTrueMotionMode,
	kDummyLumaMode,
	kChromaModeCount = kDummyLumaMode, kLumaModeCount
};

enum SubblockMode {
	kAverageSubmode, kTrueMotionSubmode, kVerticalSubmode,
	kHorizontalSubmode, kLeftDownSubmode, kRightDownSubmode,
	kVerticalRightSubmode, kVerticalLeftSubmode, kHorizontalDownSubmode,
	kHorizontalUpSubmode, kSubmodeCount
};

enum MiscConst {
	kCoeffTokenCount = 12, kSegmentCount = 4, kBlockSize = 16
};

} // namespace cherry;

#endif // _CHERRY_VP8_CONST_HPP_INCLUDED_
