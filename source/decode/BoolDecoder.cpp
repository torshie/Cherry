#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/wrapper/wrapper.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/ProbabilityManager.hpp>

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

//        +------root--------+
//        |                  |
//    kAverageMode   +-------1-------+
//                   |               |
//         kVerticalMode     +-------2-----------+
//                           |                   |
//                     kHorizontalMode      kTrueMotionMode
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


//  Coefficient token tree:
//   +--(0)---+
//   |        |
// (End)  +--(1)---+
//        |        |                        +-------- This subtree is
//        0  +----(2)------+                |         decoded by method
//           |             |                V         decodeLargeCoeff()
//           1   +--------(3)---------------+
//               |                          |
//     +--------(4)---+             +------(6)-------+
//     |              |             |                |
//     2       +-----(5)--+     +--(7)--+     +-----(8)------+
//             |          |     |       |     |              |
//             3          4    5~6     7~10   |              |
//                                      +----(9)--+     +---(10)--+
//                                      |         |     |         |
//                                   11~18     19~34  35~66    67~2048
int BoolDecoder::decodeCoeff(short* coefficient, bool acOnly, int plane,
		int context, ProbabilityManager* pm) {
	const static int kZigZag[] = {
		0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15
	};
	const static int kCoeffBand[] = {
		0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 0
	};

	coefficient[0] = 0;
	int index = acOnly ? 1 : 0;
	Probability* probability = pm->coeff[plane][index][context];
	if (!decode(probability[0])) {
		return 0;
	}
	for (; index < 16; ++index) {
		if (!decode(probability[1])) {
			probability = pm->coeff[plane][kCoeffBand[index + 1]][0];
			continue;
		}
		int value;
		if (!decode(probability[2])) {
			probability = pm->coeff[plane][kCoeffBand[index + 1]][1];
			value = 1;
		} else {
			if (!decode(probability[3])) {
				if (!decode(probability[4])) {
					value = 2;
				} else {
					if (!decode(probability[5])) {
						value = 3;
					} else {
						value = 4;
					}
				}
			} else {
				value = decodeLargeCoeff(probability);
			}
			probability = pm->coeff[plane][kCoeffBand[index + 1]][2];
		}
		coefficient[kZigZag[index]] = uint<1>() ? -value : value;
		if (index == 15 || !decode(probability[0])) {
			++index;
			break;
		}
	}
	return index;
}

int BoolDecoder::decodeLargeCoeff(const Probability* probability) {
	if (!decode(probability[6])) {
		if (!decode(probability[7])) {
			return 5 + decodeTokenOffset(0);
		} else {
			return 7 + decodeTokenOffset(1);
		}
	} else {
		if (!decode(probability[8])) {
			if (!decode(probability[9])) {
				return 11 + decodeTokenOffset(2);
			} else {
				return 19 + decodeTokenOffset(3);
			}
		} else {
			if (!decode(probability[10])) {
				return 35 + decodeTokenOffset(4);
			} else {
				return 67 + decodeTokenOffset(5);
			}
		}
	}
}

short BoolDecoder::decodeTokenOffset(int category) {
	const static Probability kOffsetProbability[][12] = {
		{ 159, 0},
		{ 165, 145, 0},
		{ 173, 148, 140, 0},
		{ 176, 155, 140, 135, 0},
		{ 180, 157, 141, 134, 130, 0},
		{ 254, 254, 243, 230, 196, 177, 153, 140, 133, 130, 129, 0}
	};
	int_fast16_t offset = 0;
	for (const Probability* p = kOffsetProbability[category]; *p != 0;
			++p) {
		offset += offset + decode(*p);
	}
	return offset;
}
