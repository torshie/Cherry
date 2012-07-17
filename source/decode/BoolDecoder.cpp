#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/wrapper/wrapper.hpp>
#include <cherry/vp8/const.hpp>

using namespace cherry;

void BoolDecoder::reload(const void* input, size_t size) {
	if (size <= 2) {
		RAISE(FeatureIncomplete, "String fed to BoolDecoder is too short");
	}
	shift = 0;
	length = 255;
	buffer = (const uint8_t*)input;
	cursor = buffer + 2;
	this->size = size;
	value = ((uint32_t)buffer[0] << 8) + buffer[1];
}

//             +----root--+
//             |          |
//     kDummyLumaMode     |
//                  +-----1------------------+
//                  |                        |
//            +-----2-----+            +-----3----------+
//            |           |            |                |
//    kAverageMode   kVerticalMode  kHorizontalMode   kTrueMotionMode
IntraMode BoolDecoder::decodeLumaMode() {
	const static Probability kProbability[] = {
		145, 156, 163, 128
	};
	if (!decode(kProbability[0])) {
		return kDummyLumaMode;
	}
	if (!decode(kProbability[1])) {
		if (!decode(kProbability[2])) {
			return kAverageMode;
		} else {
			return kVerticalMode;
		}
	}
	if (!decode(kProbability[3])) {
		return kHorizontalMode;
	} else {
		return kTrueMotionMode;
	}
}

//              +------root--------+
//              |                  |
//          kAverageMode   +-------1-------+
//                         |               |
//               kVerticalMode     +-------2-----------+
//                                 |                   |
//                            kHorizontalMode      kTrueMotionMode
IntraMode BoolDecoder::decodeChromaMode() {
	const static Probability kProbability[] = {
		142, 114, 183
	};
	if (!decode(kProbability[0])) {
		return kAverageMode;
	}
	if (!decode(kProbability[1])) {
		return kVerticalMode;
	}
	if (!decode(kProbability[2])) {
		return kHorizontalMode;
	} else {
		return kTrueMotionMode;
	}
}

//             +---root---+
//             |          |
// kAverageSubmode   +----1-----+
//                   |          |
//    kTrueMotionSubmode   +----2----+
//                         |         |
//            kVerticalSubmode   +---3----------+
//                               |              |
//                           +---4---+       +--6-------+
//                           |       |       |          |
//            kHorizontalSubmode     | kLeftDownSubmode |
//               +-------------------5--+               |
//               |                      |       +-------7--+
//  kRightDownSubmode  kVerticalRightSubmode    |          |
//                                              |          |
//                             kVerticalLeftSubmode        |
//                                     +-------------------8--+
//                                     |                      |
//                      kHorizontalDownSubmode      kHorizontalUpSubmode
SubblockMode BoolDecoder::decodeSubblockMode(
		const Probability* probability) {
	if (!decode(probability[0])) {
		return kAverageSubmode;
	}
	if (!decode(probability[1])) {
		return kTrueMotionSubmode;
	}
	if (!decode(probability[2])) {
		return kVerticalSubmode;
	}
	if (!decode(probability[3])) {
		if (!decode(probability[4])) {
			return kHorizontalSubmode;
		}
		if (!decode(probability[5])) {
			return kRightDownSubmode;
		} else {
			return kVerticalRightSubmode;
		}
	}
	if (!decode(probability[6])) {
		return kLeftDownSubmode;
	}
	if (!decode(probability[7])) {
		return kVerticalLeftSubmode;
	}
	if (!decode(probability[8])) {
		return kHorizontalDownSubmode;
	} else {
		return kHorizontalUpSubmode;
	}
}

//               +--------(0)----+
//               |               |
//          +---(1)---+    +----(2)----+
//          |         |    |           |
//          0         1    2           3
int_fast8_t BoolDecoder::decodeSegmentId(const Probability* probability) {
	// TODO this function is untested.
	if (!decode(probability[0])) {
		if (!decode(probability[1])) {
			return 0;
		} else {
			return 1;
		}
	}
	if (!decode(probability[2])) {
		return 2;
	} else {
		return 3;
	}
}
