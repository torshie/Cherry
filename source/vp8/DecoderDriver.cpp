#include <cstdlib>
#include <cherry/utility/misc.hpp>
#include <cherry/utility/memory.hpp>
#include <cherry/vp8/DecoderDriver.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/except/InvalidInputStream.hpp>
#include <cherry/except/Unpossible.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/dsp/dsp.hpp>

using namespace cherry;


DecoderDriver::DecoderDriver() : frameData(NULL), frameSize(0) {
	const static Probability
	kDefaultCoeff[4][8][3][kCoeffTokenCount - 1] = {
#include <table/probability/defaultCoefficient.i>
	};
	std::memset(&context, 0, sizeof(context));
	std::memset(&geometry, 0, sizeof(geometry));
	std::memcpy(&context.coeff, &kDefaultCoeff, sizeof(kDefaultCoeff));
	for (size_t i = 0; i < ELEMENT_COUNT(context.quantizer.index); ++i) {
		context.quantizer.index[i] = -1;
	}
}

void DecoderDriver::decodeFrame() {
	decodeFrameHeader();

	for (int i = 1; i < geometry.blockHeight; ++i) {
		for (int j = 1; j < geometry.blockWidth; ++j) {
			int tmp = geometry.blockWidth * i + j;
			decodeBlockHeader(buffer.current.info + tmp);
		}
	}

	int tagSize = context.keyFrame ? 10 : 3;
	source.reload((char*)frameData + context.firstPartSize + tagSize,
			frameSize - context.firstPartSize - tagSize);

	for (int i = 1; i < geometry.blockHeight; ++i) {
		for (int j = 1; j < geometry.blockWidth; ++j) {
			decodeMacroblock(i, j);
		}
	}

	// TODO apply loop filters

	writeDisplayBuffer();
}

void DecoderDriver::setFrameData(const void* data, size_t size) {
	frameData = data;
	frameSize = size;

	decodeFrameTag(data);
	int tagSize = context.keyFrame ? 10 : 3;
	source.reload((const char*)data + tagSize, size - tagSize);
}

Pixel DecoderDriver::clamp(int value) {
	if (value < 0) {
		return 0;
	} else if (value > 255) {
		return 255;
	} else {
		return value;
	}
}

void DecoderDriver::decodeFrameHeader() {
	if (context.keyFrame) {
		if (source.uint<1>() != 0) {
			RAISE(InvalidInputStream, "Color space other than YUV isn't "
					"supported.");
		}
		context.disableClamping = source.uint<1>();
	}
	updateSegmentation();
	updateLoopFilter();
	context.log2PartitionCount = source.uint<2>();
	if (context.log2PartitionCount != 0) {
		RAISE(FeatureIncomplete, "Partitioning isn't supported");
	}
	decodeQuantizerTable();
	if (context.keyFrame) {
		context.refreshProbability = source.uint<1>();
	} else {
		// TODO: Complete this branch
	}
	updateCoeffProbability();
	context.skipping.enabled = source.uint<1>();
	if (context.skipping.enabled) {
		context.skipping.probability = source.uint<8>();
	}
	if (!context.keyFrame) {
		// TODO: Complete this branch
	}
}

void DecoderDriver::decodeBlockHeader(BlockInfo* info) {
	if (context.segment.enabled && context.segment.updateMapping) {
		info->segment = source.decodeSegmentId(
				context.segment.probability);
	}
	if (context.skipping.enabled) {
		info->skipCoeff = source.decode(context.skipping.probability);
	}
	// TODO: Handle inter frames.
	info->lumaMode = source.decodeLumaMode();
	if (info->lumaMode == kDummyLumaMode) {
		decodeSubmode(info);
	} else {
		info->setDummySubmode();
	}
	info->chromaMode = source.decodeChromaMode();
}

void DecoderDriver::decodeSubmode(BlockInfo* info) {
	const static Probability
	probability[kSubmodeCount][kSubmodeCount][kSubmodeCount - 1] = {
#include <table/probability/keyFrame/submode.i>
	};

	BlockInfo* blockAbove = info - geometry.blockWidth;
	int8_t* above = blockAbove->submode;
	int8_t* left = (info - 1)->submode;
	int8_t* dest = info->submode;
	dest[0] = source.decodeSubblockMode(probability[above[12]][left[3]]);
	for (int i = 1; i < 4; ++i) {
		dest[i] = source.decodeSubblockMode(
				probability[above[12 + i]][dest[i - 1]]);
	}
	for (int i = 4; i < 16; ++i) {
		if (i % 4 == 0) {
			dest[i] = source.decodeSubblockMode(
					probability[dest[i - 4]][left[i + 3]]);
		} else {
			dest[i] = source.decodeSubblockMode(
					probability[dest[i - 4]][dest[i - 1]]);
		}
	}
}

short DecoderDriver::decodeTokenOffset(int category) {
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
		offset += offset + source.decode(*p);
	}
	return offset;
}

void DecoderDriver::decodeFrameTag(const void* data) {
	const uint8_t* cursor = (const uint8_t*)data;
	uint32_t raw = cursor[0] | ((uint32_t)cursor[1] << 8)
			| ((uint32_t)cursor[2] << 16);
	cursor += 3;
	context.keyFrame = !(raw & 1);
	context.version = (raw >> 1) & 7;
	context.invisible = !((raw >> 4) & 1);
	context.firstPartSize = (raw >> 5) & 0x7ffff;
	if (context.keyFrame) {
		if (cursor[0] != 0x9d || cursor[1] != 0x01 || cursor[2] != 0x2a) {
			RAISE(InvalidInputStream,
					"Cannot find start code in key frame.");
		}
		cursor += 3;
		uint16_t tmp = cursor[0] | ((uint16_t)cursor[1] << 8);
		cursor += 2;
		int width = tmp & 0x3fff;
		tmp = cursor[0] | ((uint16_t)cursor[1] << 8);
		int height = tmp & 0x3fff;
		if (width != geometry.displayWidth
				|| height != geometry.displayHeight) {
			resizeFrame(width, height);
		}
	}
}

void DecoderDriver::addSubblockResidual(int row, int column, Pixel* target,
		const short* residual, Pixel subblock[4][4]) {
	for (int i = 0; i < 4; ++i) {
		int offset = (row * 4 + i) * geometry.lumaWidth + column * 4;
		for (int j = 0; j < 4; ++j) {
			target[offset + j] =
					clamp(subblock[i][j] + residual[i * 4 + j]);
		}
	}
}

//  Coefficient token tree:
//   +--(0)---+
//   |        |
// (End)  +--(1)---+
//        |        |
//        0  +----(2)------+                       +-- This subtree is
//           |             |                       |   decoded by method
//           1   +--------(3)---------------+  <<--+   decodeLargeCoeff()
//               |                          |
//     +--------(4)---+             +------(6)-------+
//     |              |             |                |
//     2       +-----(5)--+     +--(7)--+     +-----(8)------+
//             |          |     |       |     |              |
//             3          4    5~6     7~10   |              |
//                                      +----(9)--+     +---(10)--+
//                                      |         |     |         |
//                                   11~18     19~34  35~66    67~2048
int DecoderDriver::decodeCoeffArray(short* coefficient, bool acOnly,
		PlaneType plane, int ctx) {
	const static int kZigZag[] = {
		0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
	};
	const static int kCoeffBand[] = {
		0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 0
	};

	coefficient[0] = 0;
	int index = acOnly ? 1 : 0;
	Probability* probability = context.coeff[plane][index][ctx];
	if (!source.decode(probability[0])) {
		return 0;
	}
	for (; index < 16; ++index) {
		if (!source.decode(probability[1])) {
			probability = context.coeff[plane][kCoeffBand[index + 1]][0];
			continue;
		}
		int value;
		if (!source.decode(probability[2])) {
			probability = context.coeff[plane][kCoeffBand[index + 1]][1];
			value = 1;
		} else {
			if (!source.decode(probability[3])) {
				if (!source.decode(probability[4])) {
					value = 2;
				} else {
					if (!source.decode(probability[5])) {
						value = 3;
					} else {
						value = 4;
					}
				}
			} else {
				value = decodeLargeCoeff(probability);
			}
			probability = context.coeff[plane][kCoeffBand[index + 1]][2];
		}
		coefficient[kZigZag[index]] = source.uint<1>() ? -value : value;
		if (index == 15 || !source.decode(probability[0])) {
			++index;
			break;
		}
	}
	dequantizeCoefficient(coefficient, plane, index);
	return index;
}

int DecoderDriver::decodeLargeCoeff(const Probability* probability) {
	if (!source.decode(probability[6])) {
		if (!source.decode(probability[7])) {
			return 5 + decodeTokenOffset(0);
		} else {
			return 7 + decodeTokenOffset(1);
		}
	} else {
		if (!source.decode(probability[8])) {
			if (!source.decode(probability[9])) {
				return 11 + decodeTokenOffset(2);
			} else {
				return 19 + decodeTokenOffset(3);
			}
		} else {
			if (!source.decode(probability[10])) {
				return 35 + decodeTokenOffset(4);
			} else {
				return 67 + decodeTokenOffset(5);
			}
		}
	}
}

void DecoderDriver::decodeBlockCoeff(short coefficient[25][16],
		BlockInfo* info, int row, int column) {
	std::memset(coefficient, 0, 25 * 16 * sizeof(short));
	if (context.skipping.enabled && info->skipCoeff) {
		std::memset(context.hasCoeff.above[column], 0, sizeof(bool) * 8);
		std::memset(context.hasCoeff.left[row], 0, sizeof(bool) * 8);
		if (info->hasVirtualPlane()) {
			context.hasCoeff.above[column][8] = false;
			context.hasCoeff.left[row][8] = false;
		}
		return;
	}
	bool acOnly = false;
	PlaneType lumaType = kFullLumaPlane;
	if (info->hasVirtualPlane()) {
		int ctx = context.hasCoeff.above[column][8]
				+ context.hasCoeff.left[row][8];
		info->lastCoeff[24] = decodeCoeffArray(coefficient[24], false,
				kVirtualPlane, ctx);
		context.hasCoeff.above[column][8] =
				context.hasCoeff.left[row][8] =
						(info->lastCoeff[24] > 0);
		acOnly = true;
		lumaType = kPartialLumaPlane;
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			int ctx = context.hasCoeff.above[column][j]
					+ context.hasCoeff.left[row][i];
			int id = i * 4 + j;
			info->lastCoeff[id] = decodeCoeffArray(coefficient[id],
					acOnly, lumaType, ctx);
			context.hasCoeff.above[column][j] =
					context.hasCoeff.left[row][i] =
							(info->lastCoeff[id] > 0);
		}
	}
	for (int p = 0; p < 2; ++p) {
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 2; ++j) {
				int ctx = context.hasCoeff.above[column][4 + p * 2 + j]
						+ context.hasCoeff.left[row][4 + p * 2 + i];
				int id = 16 + 4 * p + 2 * i + j;
				info->lastCoeff[id] = decodeCoeffArray(coefficient[id],
						false, kChromaPlane, ctx);
				context.hasCoeff.above[column][4 + p * 2 + j] =
						context.hasCoeff.left[row][4 + p * 2 + i] =
								(info->lastCoeff[id] > 0);
			}
		}
	}
}

void DecoderDriver::updateLoopFilter() {
	context.filter.type = source.uint<1>();
	context.filter.level = source.uint<6>();
	context.filter.sharpness = source.uint<3>();
	context.filter.delta.enabled = source.uint<1>();
	if (context.filter.delta.enabled && source.uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			if (source.uint<1>()) {
				context.filter.delta.reference[i] = source.sint<7>();
			}
		}
		for (int i = 0; i < 4; ++i) {
			if (source.uint<1>()) {
				context.filter.delta.mode[i] = source.sint<7>();
			}
		}
	}
}

void DecoderDriver::updateSegmentation() {
	context.segment.enabled = source.uint<1>();
	if (!context.segment.enabled) {
		return;
	}
	RAISE(FeatureIncomplete, "Segmentation isn't supported");
	context.segment.updateMapping = source.uint<1>();
	bool updateFeature = source.uint<1>();
	if (updateFeature) {
		bool absoluteValue = source.uint<1>();
		for (int i = 0; i < 4; ++i) {
			int8_t value = 0;
			if (source.uint<1>()) {
				value = source.sint<8>();
			}
			if (absoluteValue) {
				context.segment.quantizer[i] = value;
			} else {
				context.segment.quantizer[i] += value;
			}
		}
		for (int i = 0; i < 4; ++i) {
			int8_t value = 0;
			if (source.uint<1>()) {
				value = source.sint<7>();
			}
			if (absoluteValue) {
				context.segment.loopFilter[i] = value;
			} else {
				context.segment.loopFilter[i] += value;
			}
		}
	}
	if (context.segment.updateMapping) {
		for (size_t i = 0; i < ELEMENT_COUNT(context.segment.probability);
				++i) {
			Probability probability = 255;
			if (source.uint<1>()) {
				probability = source.uint<8>();
			}
			context.segment.probability[i] = probability;
		}
	}
}

void DecoderDriver::decodeQuantizerTable() {
	bool update = false;
	int v = source.uint<7>();
	if (context.quantizer.index[0] != v) {
		context.quantizer.index[0] = v;
		update = true;
	}
	for (int i = 1; i < kQuantizerCount; ++i) {
		v = context.quantizer.index[0];
		if (source.uint<1>()) {
			v += source.sint<5>();
			if (v < 0) {
				v = 0;
			} else if (v > 127) {
				v = 127;
			}
		}
		if (context.quantizer.index[i] != v) {
			context.quantizer.index[i] = v;
			update = true;
		}
	}

	if (update) {
		updateQuantizerValue();
	}
}

void DecoderDriver::updateQuantizerValue() {
	static const short kDcQuantizer[] = {
#include <table/quantizer/dc.i>
	};

	int* index = context.quantizer.index;
	short (*value)[2] = context.quantizer.value;
	value[kLumaDc / 2][0] = kDcQuantizer[index[kLumaDc]];
	value[kVirtualDc / 2][0] = kDcQuantizer[index[kVirtualDc]] * 2;
	short factor = kDcQuantizer[index[kChromaDc]];
	value[kChromaDc / 2][0] = (factor > 132) ? 132 : factor;

	static const short kAcQuantizer[] = {
#include <table/quantizer/ac.i>
	};
	value[kLumaAc / 2][1] = kAcQuantizer[index[kLumaAc]];
	factor = kAcQuantizer[index[kVirtualAc]] * 155 / 100;
	if (factor < 8) {
		factor = 8;
	}
	value[kVirtualAc / 2][1] = factor;
	value[kChromaAc / 2][1] = kAcQuantizer[index[kChromaAc]];
}

void DecoderDriver::updateCoeffProbability() {
	const static Probability
	kProbability[4][8][3][kCoeffTokenCount - 1] = {
#include <table/probability/updateCoefficient.i>
	};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 8; ++j) {
			for (int k = 0; k < 3; ++k) {
				for (int l = 0; l < kCoeffTokenCount - 1; ++l) {
					if (source.decode(kProbability[i][j][k][l])) {
						context.coeff[i][j][k][l] = source.uint<8>();
					}
				}
			}
		}
	}
}

void DecoderDriver::update16x16Probability() {
	if (source.uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			source.uint<8>();
		}
	}
}

void DecoderDriver::updateChromaProbability() {
	if (source.uint<1>()) {
		for (int i = 0; i < 3; ++i) {
			source.uint<8>();
		}
	}
}

void DecoderDriver::updateMotionVectorProbability() {
	for (int i = 0; i < 2 * 19; ++i) {
		if (source.uint<1>()) {
			source.uint<7>();
		}
	}
}

void DecoderDriver::resizeFrame(int width, int height) {
	geometry.displayWidth = width;
	geometry.displayHeight = height;
	geometry.blockWidth = (width + 16 - 1) / 16 + 1;
	geometry.lumaWidth = geometry.blockWidth * 16;
	geometry.blockHeight = (height + 16 - 1) / 16 + 1;
	geometry.lumaHeight = geometry.blockHeight * 16;
	geometry.chromaWidth = geometry.lumaWidth / 2;
	geometry.chromaHeight = geometry.lumaHeight / 2;

	display.destroy();
	display.create(width, height);

	buffer.current.destroy();
	buffer.current.create(geometry.blockWidth, geometry.blockHeight);
	for (int i = 0; i < geometry.blockWidth; ++i) {
		buffer.current.info[i].initForPadding();
	}
	for (int i = 1; i < geometry.blockHeight; ++i) {
		buffer.current.info[i * geometry.blockWidth].initForPadding();
	}

	context.hasCoeff.destroy();
	context.hasCoeff.create(geometry.blockWidth, geometry.blockHeight);
}

void DecoderDriver::decodeMacroblock(int row, int column) {
	short coefficient[25][16];
	BlockInfo* info = buffer.current.info + row * geometry.blockWidth
			+ column;
	decodeBlockCoeff(coefficient, info, row, column);

	predictLumaBlock(row, column, info, coefficient);
	predictChromaBlock(row, column, info, coefficient);
}

void DecoderDriver::predictLumaBlock(int row, int column, BlockInfo* info,
		short coefficient[25][16]) {
	Pixel* above = buffer.current.luma + column * 16
			+ geometry.lumaWidth * (row * 16 - 1);
	Pixel* target = above + geometry.lumaWidth;
	Pixel* left = target - 1;
	if (info->lumaMode == kDummyLumaMode) {
		if (column == geometry.blockWidth - 1) {
			Pixel extra[4];
			std::memset(extra, above[15], sizeof(extra));
			predictBusyLuma(above, left, extra, target, info, coefficient);
		} else {
			predictBusyLuma(above, left, above + 16, target, info,
					coefficient);
		}
		return;
	}

	predictWholeBlock<16>(row, column, above, left, target,
			info->lumaMode);
	short residual[16], dc[16];
	if (info->lastCoeff[24] > 0) {
		inverseWalshHadamardTransform(coefficient[24], dc);
	} else {
		std::memset(dc, 0, sizeof(dc));
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (dc[i * 4 + j] == 0 && info->lastCoeff[i * 4 + j] == 0) {
				continue;
			}
			coefficient[i * 4 + j][0] = dc[i * 4 + j];
			inverseDiscreteCosineTransform(coefficient[i * 4 + j],
					residual);
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					Pixel* p = target + (i * 4 + y) * geometry.lumaWidth
							+ j * 4 + x;
					*p = clamp(*p + residual[y * 4 + x]);
				}
			}
		}
	}
}

void DecoderDriver::predictBusyLuma(Pixel* above, Pixel* left,
		Pixel* extra, Pixel* target, BlockInfo* info,
		short coefficient[25][16]) {
	for (int row = 0; row < 4; ++row, above += 4 * geometry.lumaWidth) {
		Pixel subblock[4][4];
		short residual[16];
		left = above + geometry.lumaWidth - 1;
		for (int col = 0; col < 3; ++col, left += 4) {
			predict4x4(subblock, above + col * 4, left,
					above + col * 4 + 4, info->submode[row * 4 + col]);
			inverseDiscreteCosineTransform(coefficient[row * 4 + col],
					residual);
			addSubblockResidual(row, col, target, residual, subblock);
		}
		predict4x4(subblock, above + 12, left, extra,
				info->submode[row * 4 + 3]);
		inverseDiscreteCosineTransform(coefficient[4 * row + 3], residual);
		addSubblockResidual(row, 3, target, residual, subblock);
	}
}

void DecoderDriver::predict4x4(Pixel b[4][4], Pixel* above, Pixel* left,
		Pixel* extra, int8_t submode) {
	Pixel E[9];
	int lineSize = geometry.lumaWidth;
	E[0] = left[3 * lineSize]; E[1] = left[2 * lineSize];
	E[2] = left[lineSize]; E[3] = left[0];
	E[4] = above[-1]; E[5] = above[0]; E[6] = above[1]; E[7] = above[2];
	E[8] = above[3];
	switch (submode) {
	case kAverageSubmode: {
		short sum = 0;
		for (int i = 0; i < 4; ++i) {
			sum += above[i];
		}
		for (int i = 0; i < 4; ++i) {
			sum += left[i * lineSize];
		}
		std::memset(b, (sum + 4) / 8, 16);
		break; }
	case kHorizontalSubmode: {
		Pixel pixel = avg3(left[-lineSize], left[0], left[lineSize]);
		for (int j = 0; j < 4; ++j) {
			b[0][j] = pixel;
		}
		for (int i = 1; i < 3; ++i) {
			pixel = avg3(left[(i - 1) * lineSize], left[i * lineSize],
					left[(i + 1) * lineSize]);
			for (int j = 0; j < 4; ++j) {
				b[i][j] = pixel;
			}
		}
		pixel = avg3(left[2 * lineSize], left[3 * lineSize],
				left[3 * lineSize]);
		for (int j = 0; j < 4; ++j) {
			b[3][j] = pixel;
		}
		break; }
	case kVerticalSubmode:
		for (int i = 0; i < 4; ++i) {
			b[i][0] = avg3p(above);
			for (int j = 1; j < 3; ++j) {
				b[i][j] = avg3p(above + j);
			}
			b[i][3] = avg3(above[2], above[3], extra[0]);
		}
		break;
	case kTrueMotionSubmode:
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				b[i][j] = clamp(above[j] + left[i * lineSize] - above[-1]);
			}
		}
		break;
	case kLeftDownSubmode:
		b[0][0] = avg3(above[0], above[1], above[2]);
		b[0][1] = b[1][0] = avg3(above[1], above[2], above[3]);
		b[0][2] = b[1][1] = b[2][0] =
				avg3(above[2], above[3], extra[0]);
		b[0][3] = b[1][2] = b[2][1] = b[3][0] =
				avg3(above[3], extra[0], extra[1]);
		b[1][3] = b[2][2] = b[3][1] =
				avg3(extra[0], extra[1], extra[2]);
		b[2][3] = b[3][2] = avg3(extra[1], extra[2], extra[3]);
		b[3][3] = avg3(extra[2], extra[3], extra[3]);
		break;
	case kRightDownSubmode:
		b[3][0] = avg3p(E + 1);
		b[3][1] = b[2][0] = avg3p(E + 2);
		b[3][2] = b[2][1] = b[1][0] = avg3p(E + 3);
		b[3][3] = b[2][2] = b[1][1] = b[0][0] =
				avg3p(E + 4);
		b[2][3] = b[1][2] = b[0][1] = avg3p(E + 5);
		b[1][3] = b[0][2] = avg3p(E + 6);
		b[0][3] = avg3p(E + 7);
		break;
	case kHorizontalDownSubmode:
		b[3][0] = avg2p(E);
		b[3][1] = avg3p(E + 1);
		b[2][0] = b[3][2] = avg2p(E + 1);
		b[2][1] = b[3][3] = avg3p(E + 2);
		b[2][2] = b[1][0] = avg2p(E + 2);
		b[2][3] = b[1][1] = avg3p(E + 3);
		b[1][2] = b[0][0] = avg2p(E + 3);
		b[1][3] = b[0][1] = avg3p(E + 4);
		b[0][2] = avg3p(E + 5);
		b[0][3] = avg3p(E + 6);
		break;
	case kHorizontalUpSubmode:
		b[0][0] = avg2(left[0], left[lineSize]);
		b[0][1] = avg3(left[0], left[lineSize], left[lineSize * 2]);
		b[0][2] = b[1][0] = avg2(left[lineSize], left[lineSize * 2]);
		b[0][3] = b[1][1] = avg3(left[lineSize], left[lineSize * 2],
				left[lineSize * 3]);
		b[1][2] = b[2][0] = avg2(left[lineSize * 2], left[lineSize * 3]);
		b[1][3] = b[2][1] = avg3(left[lineSize * 2], left[lineSize * 3],
				left[lineSize * 3]);
		b[2][2] = b[2][3] = b[3][0] = b[3][1] = b[3][2] = b[3][3] =
				left[lineSize * 3];
		break;
	case kVerticalLeftSubmode:
		b[0][0] = avg2p(above);
		b[1][0] = avg3p(above + 1);
		b[2][0] = b[0][1] = avg2p(above + 1);
		b[1][1] = b[3][0] = avg3p(above + 2);
		b[2][1] = b[0][2] = avg2p(above + 2);
		b[3][1] = b[1][2] = avg3(above[2], above[3], extra[0]);
		b[2][2] = b[0][3] = avg2(above[3], extra[0]);
		b[3][2] = b[1][3] = avg3(above[3], extra[0], extra[1]);
		b[2][3] = avg3p(extra + 1);
		b[3][3] = avg3p(extra + 2);
		break;
	case kVerticalRightSubmode:
		b[3][0] = avg3p(E + 2);
		b[2][0] = avg3p(E + 3);
		b[3][1] = b[1][0] = avg3p(E + 4);
		b[2][1] = b[0][0] = avg2p(E + 4);
		b[3][2] = b[1][1] = avg3p(E + 5);
		b[2][2] = b[0][1] = avg2p(E + 5);
		b[3][3] = b[1][2] = avg3p(E + 6);
		b[2][3] = b[0][2] = avg2p(E + 6);
		b[1][3] = avg3p(E + 7);
		b[0][3] = avg2p(E + 7);
		break;
	default:
		RAISE(Unpossible);
	}
}

void DecoderDriver::predictChromaBlock(int row, int column,
		BlockInfo* info, short coefficient[25][16]) {
	for (int p = 0; p < 2; ++p) {
		Pixel* above = buffer.current.chroma[p] + column * 8
				+ (row * 8 - 1) * geometry.chromaWidth;
		Pixel* target = above + geometry.chromaWidth;
		Pixel* left = target - 1;
		predictWholeBlock<8>(row, column, above, left, target,
				info->chromaMode);
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 2; ++j) {
				short residual[16];
				int id = 16 + p * 4 + i * 2 + j;
				inverseDiscreteCosineTransform(coefficient[id], residual);
				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int offset = (i * 4 + y) * geometry.chromaWidth
								+ j * 4 + x;
						target[offset] = clamp(
								target[offset] + residual[y * 4 + x]);
					}
				}
			}
		}
	}
}

void DecoderDriver::dequantizeCoefficient(short* coefficient,
		PlaneType plane, int lastCoeff) {
	int index = (plane == kFullLumaPlane) ? 0 : plane;
	if (plane != kPartialLumaPlane) {
		coefficient[0] *= context.quantizer.value[index][0];
	}
	const static int kLargestOffset[] = {
		0, 1, 2, 5, 9, 9, 9, 9, 9, 10, 13, 14, 14, 14, 14, 15, 16
	};
	int end = kLargestOffset[lastCoeff];
	for (int i = 1; i < end; ++i) {
		coefficient[i] *= context.quantizer.value[index][1];
	}
	for (int i = end; i < 16; ++i) {
		coefficient[i] = 0;
	}
}

void DecoderDriver::writeDisplayBuffer() {
	for (int i = 0; i < geometry.displayHeight; ++i) {
		std::memcpy(display.luma + i * geometry.displayWidth,
				buffer.current.luma + (i + 16) * geometry.lumaWidth + 16,
				geometry.displayWidth);
	}
	for (int p = 0; p < 2; ++p) {
		for (int i = 0; i < geometry.displayHeight / 2; ++i) {
			Pixel* src = buffer.current.chroma[p] + 8
					+ (i + 8) * geometry.chromaWidth;
			Pixel* dst = display.chroma[p]
					+ i * geometry.displayWidth / 2;
			std::memcpy(dst, src, geometry.displayWidth / 2);
		}
	}
}

template<int BLOCK_SIZE>
void DecoderDriver::predictWholeBlock(int row, int column, Pixel* above,
		Pixel* left, Pixel* target, int8_t mode) {
	int lineSize = geometry.blockWidth * BLOCK_SIZE;
	switch (mode) {
	case kAverageMode: {
		uint32_t sum = 0;
		int total = 0;
		if (row > 1) {
			for (int i = 0; i < BLOCK_SIZE; ++i) {
				sum += above[i];
			}
			sum += BLOCK_SIZE / 2;
			total += BLOCK_SIZE;
		}
		if (column > 1) {
			for (int i = 0; i < BLOCK_SIZE; ++i) {
				sum += left[i * lineSize];
			}
			sum += BLOCK_SIZE / 2;
			total += BLOCK_SIZE;
		}
		Pixel average = (total == 0) ? 128 : sum / total;
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			std::memset(target + i * lineSize, average, BLOCK_SIZE);
		}
		break; }
	case kHorizontalMode:
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			std::memset(target + lineSize * i, target[lineSize * i - 1],
					BLOCK_SIZE);
		}
		break;
	case kVerticalMode:
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			std::memcpy(target + lineSize * i, above, BLOCK_SIZE);
		}
		break;
	case kTrueMotionMode:
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			for (int j = 0; j < BLOCK_SIZE; ++j) {
				target[i * lineSize + j] =
						clamp(above[j] + left[i * lineSize] - above[-1]);
			}
		}
		break;
	default:
		RAISE(Unpossible);
	}
}
