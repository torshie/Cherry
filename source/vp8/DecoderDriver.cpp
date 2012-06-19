#include <cstdlib>
#include <cherry/utility/misc.hpp>
#include <cherry/vp8/DecoderDriver.hpp>
#include <cherry/except/InvalidBoolString.hpp>
#include <cherry/except/Unpossible.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/dsp/dsp.hpp>

using namespace cherry;

void DecoderDriver::decodeFrame() {
	decodeFrameHeader();

	for (int i = 0; i < context.vertical.blockCount; ++i) {
		for (int j = 0; j < context.horizon.blockCount; ++j) {
			int tmp = (context.horizon.blockCount + 1) * (i + 1) + j + 1;
			decodeBlockHeader(context.block[kCurrentFrame] + tmp);
		}
	}

	delete source;
	int tagSize = context.keyFrame ? 10 : 3;
	source = new BoolDecoder(
			(char*)frameData + context.firstPartSize + tagSize,
			frameSize - context.firstPartSize - tagSize);

	for (int i = 0; i < context.vertical.blockCount; ++i) {
		for (int j = 0; j < context.horizon.blockCount; ++j) {
			int tmp = (context.horizon.blockCount + 1) * (i + 1) + j + 1;
			buildMacroblock(tmp);
		}
	}

	writeDisplayBuffer();
}

void DecoderDriver::setFrameData(const void* data, size_t size) {
	delete source;
	frameData = data;
	frameSize = size;

	decodeFrameTag(data);
	int tagSize = context.keyFrame ? 10 : 3;
	source = new BoolDecoder((const char*)data + tagSize, size - tagSize);
}

int8_t DecoderDriver::getDummySubmode(int8_t intraMode) {
	switch (intraMode) {
	case kAverageMode: return kAverageSubmode;
	case kVerticalMode: return kVerticalSubmode;
	case kHorizontalMode: return kHorizontalSubmode;
	case kTrueMotionMode: return kTrueMotionSubmode;
	default: RAISE(Unpossible);
	}
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

void DecoderDriver::predictSingleSubblock(Pixel b[4][4],
		Pixel* above, Pixel* left, Pixel corner, Pixel* extra,
		int8_t submode) {
	Pixel E[9];
	E[0] = left[3]; E[1] = left[2]; E[2] = left[1]; E[3] = left[0];
	E[4] = corner; E[5] = above[0]; E[6] = above[1]; E[7] = above[2];
	E[8] = above[3];
	switch (submode) {
	case kAverageSubmode: {
		short sum = 0;
		for (int i = 0; i < 4; ++i) {
			sum += above[i];
		}
		for (int i = 0; i < 4; ++i) {
			sum += left[i];
		}
		std::memset(b, (sum + 4) / 8, 16);
		break; }
	case kHorizontalSubmode: {
		Pixel pixel = avg3(corner, left[0], left[1]);
		for (int j = 0; j < 4; ++j) {
			b[0][j] = pixel;
		}
		for (int i = 1; i < 3; ++i) {
			pixel = avg3(left[i - 1], left[i], left[i + 1]);
			for (int j = 0; j < 4; ++j) {
				b[i][j] = pixel;
			}
		}
		pixel = avg3(left[2], left[3], left[3]);
		for (int j = 0; j < 4; ++j) {
			b[3][j] = pixel;
		}
		break; }
	case kVerticalSubmode:
		for (int i = 0; i < 4; ++i) {
			b[i][0] = avg3(corner, above[0], above[1]);
			for (int j = 1; j < 3; ++j) {
				b[i][j] = avg3p(above + j);
			}
			b[i][3] = avg3(above[2], above[3], extra[0]);
		}
		break;
	case kTrueMotionSubmode:
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				b[i][j] = clamp(above[j] + left[i] - corner);
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
		b[0][0] = avg2p(left);
		b[0][1] = avg3p(left + 1);
		b[0][2] = b[1][0] = avg2p(left + 1);
		b[0][3] = b[1][1] = avg3p(left + 2);
		b[1][2] = b[2][0] = avg2p(left + 2);
		b[1][3] = b[2][1] = avg3(left[2], left[3], left[3]);
		b[2][2] = b[2][3] = b[3][0] = b[3][1] = b[3][2] = b[3][3] =
				left[3];
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

void DecoderDriver::decodeFrameHeader() {
	if (context.keyFrame) {
		if (source->uint<1>() != 0) {
			RAISE(InvalidBoolString, "Color space other than YUV isn't "
					"supported.");
		}
		context.disableClamping = source->uint<1>();
	}
	updateSegmentation();
	updateLoopFilter();
	context.log2PartitionCount = source->uint<2>();
	if (context.log2PartitionCount != 0) {
		RAISE(FeatureIncomplete, "Partitioning isn't supported");
	}
	updateQuantizerTable();
	if (context.keyFrame) {
		context.refreshProbability = source->uint<1>();
	} else {
		// TODO: Complete this branch
	}
	updateCoeffProbability();
	context.skipping.enabled = source->uint<1>();
	if (context.skipping.enabled) {
		context.skipping.probability = source->uint<8>();
	}
	if (!context.keyFrame) {
		// TODO: Complete this branch
	}
}

void DecoderDriver::decodeBlockHeader(BlockInfo* info) {
	if (context.segment.enabled && context.segment.updateMapping) {
		info->segment = source->decode(kSegmentMappingTree,
				context.segment.probability);
	}
	if (context.skipping.enabled) {
		info->skipCoeff = source->decode(context.skipping.probability);
	}
	// TODO: Handle inter frames.
	info->lumaMode = source->decode(kKeyFrameLumaModeTree,
			kKeyFrameLumaModeProbability);
	if (info->lumaMode == kDummyLumaMode) {
		decodeSubmode(info);
	} else {
		std::memset(info->submode, getDummySubmode(info->lumaMode),
				sizeof(info->submode));
	}
	info->chromaMode = source->decode(kChromaModeTree,
			kKeyFrameChromaModeProbability);
}

void DecoderDriver::decodeSubmode(BlockInfo* info) {
	BlockInfo* blockAbove = info - context.horizon.blockCount - 1;
	int8_t* above = blockAbove->submode;
	int8_t* left = (info - 1)->submode;
	int8_t* dest = info->submode;
	dest[0] = source->decode(kSubmodeTree,
			kKeyFrameSubmodeProbability[above[12]][left[3]]);
	for (int i = 1; i < 4; ++i) {
		dest[i] = source->decode(kSubmodeTree,
				kKeyFrameSubmodeProbability[above[12 + i]][dest[i - 1]]);
	}
	for (int i = 4; i < 16; ++i) {
		if (i % 4 == 0) {
			dest[i] = source->decode(kSubmodeTree,
					kKeyFrameSubmodeProbability[dest[i - 4]][left[i + 3]]);
		} else {
			dest[i] = source->decode(kSubmodeTree,
					kKeyFrameSubmodeProbability[dest[i - 4]][dest[i - 1]]);
		}
	}
}

short DecoderDriver::decodeSingleCoeff(const TreeIndex* tree,
		const Probability* probability) {
	short token = source->decode(tree, probability);
	if (token == kCoeffTokenCount - 1) {
		return kEndOfCoeff;
	} else if (token == 0) {
		return 0;
	} else if (token >= 5) {
		token = 3 + (1 << (token - 4)) + decodeTokenOffset(token);
	}
	return source->uint<1>() ? -token : token;
}

short DecoderDriver::decodeTokenOffset(int token) {
	const static Probability kOffsetProbability[][12] = {
		{ 159, 0},
		{ 165, 145, 0},
		{ 173, 148, 140, 0},
		{ 176, 155, 140, 135, 0},
		{ 180, 157, 141, 134, 130, 0},
		{ 254, 254, 243, 230, 196, 177, 153, 140, 133, 130, 129, 0}
	};
	short offset = 0;
	for (const Probability* p = kOffsetProbability[token - 5]; *p != 0;
			++p) {
		offset += offset + source->decode(*p);
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
			RAISE(InvalidBoolString,
					"Cannot find start code in key frame.");
		}
		cursor += 3;
		uint16_t tmp = cursor[0] | ((uint16_t)cursor[1] << 8);
		cursor += 2;
		context.horizon.scale = tmp >> 14;
		int width = tmp & 0x3fff;
		tmp = cursor[0] | ((uint16_t)cursor[1] << 8);
		context.vertical.scale = tmp >> 14;
		int height = tmp & 0x3fff;
		if (width != context.horizon.length
				|| height != context.vertical.length) {
			resizeFrame(width, height);
		}
	}
}

void DecoderDriver::addChromaResidual(int block) {
	BlockInfo* info = context.block[kCurrentFrame] + block;
	if (context.skipping.enabled && info->skipCoeff) {
		return;
	}
	short coefficient[16];
	for (int p = 0; p < 2; ++p) {
		FrameBuffer::Chroma* chroma =
				context.buffer[kCurrentFrame]->chroma[p];
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 2; ++j) {
				decodeCoeffArray(coefficient, false,
						17 + p * 4 + i * 2 + j, kChromaPlane, info);
				short pixel[16];
				inverseDiscreteCosineTransform(coefficient, pixel);
				for (int row = 0; row < 4; ++row) {
					for (int col = 0; col < 4; ++col) {
						int offset = i * 4 * 8 + row * 8 + j * 4 + col;
						chroma[block][offset] =
								clamp(pixel[ row * 4 + col]
										+ chroma[block][offset]);
					}
				}
			}
		}
	}
}

void DecoderDriver::decodeCoeffArray(short* coefficient, bool acOnly,
		int coeffArrayId, PlaneType plane, BlockInfo* info) {
	const static int kZigZag[] = {
		0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
	};

	std::memset(coefficient, 0, sizeof(*coefficient) * 16);

	int first = acOnly ? 1 : 0;
	short coeff = decodeSingleCoeff(kCoeffTokenTree,
			selectCoeffProbability(coeffArrayId, plane, first, info));
	if (coeff == kEndOfCoeff) {
		return;
	}
	coefficient[kZigZag[first]] = coeff;
	if (coeff != 0) {
		info->hasCoeff[coeffArrayId] = true;
	}
	short previous = coeff;
	for (int i = first + 1; i < 16; ++i, previous = coeff) {
		int index;
		switch (previous) {
		case 0: index = 0; break;
		case 1: case -1: index = 1; break;
		default: index = 2; break;
		}
		if (previous == 0) {
			coeff = decodeSingleCoeff(kSmallTokenTree,
					context.coeff[plane][kCoeffBand[i]][index] + 1);
		} else {
			coeff = decodeSingleCoeff(kCoeffTokenTree,
					context.coeff[plane][kCoeffBand[i]][index]);
		}
		if (coeff == kEndOfCoeff) {
			break;
		}
		coefficient[kZigZag[i]] = coeff;
		if (coeff != 0 && !info->hasCoeff[coeffArrayId]) {
			info->hasCoeff[coeffArrayId] = true;
		}
	}
	dequantizeCoefficient(coefficient, plane);
}

void DecoderDriver::updateLoopFilter() {
	context.filter.type = source->uint<1>();
	context.filter.level = source->uint<6>();
	context.filter.sharpness = source->uint<3>();
	context.filter.delta.enabled = source->uint<1>();
	if (context.filter.delta.enabled && source->uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			if (source->uint<1>()) {
				context.filter.delta.reference[i] = source->sint<7>();
			}
		}
		for (int i = 0; i < 4; ++i) {
			if (source->uint<1>()) {
				context.filter.delta.mode[i] = source->sint<7>();
			}
		}
	}
}

void DecoderDriver::updateSegmentation() {
	context.segment.enabled = source->uint<1>();
	if (!context.segment.enabled) {
		return;
	}
	RAISE(FeatureIncomplete, "Segmentation isn't supported");
	context.segment.updateMapping = source->uint<1>();
	bool updateFeature = source->uint<1>();
	if (updateFeature) {
		bool absoluteValue = source->uint<1>();
		for (int i = 0; i < 4; ++i) {
			int8_t value = 0;
			if (source->uint<1>()) {
				value = source->sint<8>();
			}
			if (absoluteValue) {
				context.segment.quantizer[i] = value;
			} else {
				context.segment.quantizer[i] += value;
			}
		}
		for (int i = 0; i < 4; ++i) {
			int8_t value = 0;
			if (source->uint<1>()) {
				value = source->sint<7>();
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
			if (source->uint<1>()) {
				probability = source->uint<8>();
			}
			context.segment.probability[i] = probability;
		}
	}
}

void DecoderDriver::updateQuantizerTable() {
	context.quantizer[0] = source->uint<7>();
	for (int i = 1; i < kQuantizerCount; ++i) {
		context.quantizer[i] = context.quantizer[0];
		if (source->uint<1>()) {
			int v = context.quantizer[0] + source->sint<5>();
			if (v < 0) {
				v = 0;
			} else if (v > 127) {
				v = 127;
			}
			context.quantizer[i] = v;
		}
	}
}

void DecoderDriver::updateCoeffProbability() {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 8; ++j) {
			for (int k = 0; k < 3; ++k) {
				for (int l = 0; l < kCoeffTokenCount - 1; ++l) {
					if (source->decode(
							kCoeffUpdateProbability[i][j][k][l])) {
						context.coeff[i][j][k][l] = source->uint<8>();
					}
				}
			}
		}
	}
}

void DecoderDriver::update16x16Probability() {
	if (source->uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			source->uint<8>();
		}
	}
}

void DecoderDriver::updateChromaProbability() {
	if (source->uint<1>()) {
		for (int i = 0; i < 3; ++i) {
			source->uint<8>();
		}
	}
}

void DecoderDriver::updateMotionVectorProbability() {
	for (int i = 0; i < 2 * 19; ++i) {
		if (source->uint<1>()) {
			source->uint<7>();
		}
	}
}

void DecoderDriver::resizeFrame(int width, int height) {
	context.horizon.length = width;
	context.horizon.blockCount = (width + kBlockWidth - 1) / kBlockWidth;
	context.vertical.length = height;
	context.vertical.blockCount = (height + kBlockWidth - 1) / kBlockWidth;
	int frameBlockCount = (context.vertical.blockCount + 1)
			* (context.horizon.blockCount + 1);
	for (size_t i = 0; i < ELEMENT_COUNT(context.block); ++i) {
		delete[] context.block[i];
		context.block[i] = new BlockInfo[frameBlockCount];
	}
	for (int i = 0; i < context.horizon.blockCount + 1; ++i) {
		context.block[kCurrentFrame][i].initForPadding();
	}
	for (int i = context.horizon.blockCount + 1; i < frameBlockCount;
			i += context.horizon.blockCount + 1) {
		context.block[kCurrentFrame][i].initForPadding();
	}

	for (size_t i = 0; i < ELEMENT_COUNT(context.buffer); ++i) {
		delete context.buffer[i];
		context.buffer[i] = new FrameBuffer(context.horizon.blockCount,
				context.vertical.blockCount);
	}
	for (int i = 0; i < 2; ++i) {
		FrameBuffer::Chroma* chroma =
				context.buffer[kCurrentFrame]->chroma[i];
		for (int j = 0; j < context.horizon.blockCount + 1; ++j) {
			std::memset(chroma[j], 127, sizeof(*chroma));
		}
		for (int j = 1; j < context.vertical.blockCount + 1; ++j) {
			std::memset(chroma[j * context.horizon.blockCount],
					129, sizeof(*chroma));
		}
	}

	delete[] display.luma;
	delete[] display.chroma[0];
	delete[] display.chroma[1];
	display.luma = new Pixel[width * height];
	display.chroma[0] = new Pixel[width / 2 * height / 2];
	display.chroma[1] = new Pixel[width / 2 * height / 2];
}

void DecoderDriver::buildMacroblock(int block) {
	FrameBuffer* frame = context.buffer[kCurrentFrame];

	decodeLumaBlock(frame->luma, block);

	predictChromaBlock(frame->chroma[0], block);
	predictChromaBlock(frame->chroma[1], block);
	addChromaResidual(block);
}

void DecoderDriver::decodeLumaBlock(FrameBuffer::Luma* plane, int block) {
	int offsetAbove = block - context.horizon.blockCount - 1;
	Pixel above[16], left[16];
	std::memcpy(above, &plane[offsetAbove][16 * 15], sizeof(above));
	for (int i = 0; i < 16; ++i) {
		left[i] = plane[block - 1][16 * i + 15];
	}
	BlockInfo* info = context.block[kCurrentFrame] + block;
	Pixel corner = plane[offsetAbove - 1][16 * 16 - 1];
	if (context.block[kCurrentFrame][block].lumaMode == kDummyLumaMode) {
		Pixel extra[4];
		if (block < (context.horizon.blockCount + 1) * 2) {
			std::memset(extra, 127, sizeof(extra));
		} else if (block % (context.horizon.blockCount + 1)
				== context.horizon.blockCount) {
			std::memset(extra, plane[offsetAbove][16 * 16 - 1],
					sizeof(extra));
		} else {
			std::memcpy(extra, &plane[offsetAbove + 1][16 * 15],
					sizeof(extra));
		}
		predictSubblock(plane, block, above, left, corner, extra);
		return;
	}

	predictWholeBlock<16>(block, plane[block], above, left, corner,
			context.block[kCurrentFrame][block].lumaMode);
	if (context.skipping.enabled && info->skipCoeff) {
		return;
	}
	short coefficient[16], pixel[16], dc[16];
	decodeCoeffArray(coefficient, false, 0, kVirtualPlane, info);
	inverseWalshHadamardTransform(coefficient, dc);
	for (int i = 0; i < 16; ++i) {
		decodeCoeffArray(coefficient, true, 1 + i, kPartialLumaPlane,
				info);
		coefficient[0] = dc[i];
		inverseDiscreteCosineTransform(coefficient, pixel);
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 4; ++x) {
				Pixel* p = plane[block] + i / 4 * 4 * 16 + y * 16
						+ (i % 4) * 4 + x;
				*p = clamp(*p + pixel[y * 4 + x]);
			}
		}
	}
}

void DecoderDriver::predictSubblock(FrameBuffer::Luma* plane, int block,
		Pixel* above, Pixel* left, Pixel corner, Pixel* extra) {
	BlockInfo* info = context.block[kCurrentFrame] + block;
	bool skipCoeff = context.skipping.enabled && info->skipCoeff;
	Pixel subblock[4][4];
	predictSingleSubblock(subblock, above, left, corner, above + 4,
			info->submode[0]);
	short coefficient[16], pixel[16];
	if (!skipCoeff) {
		decodeCoeffArray(coefficient, false, 1, kFullLumaPlane, info);
		inverseDiscreteCosineTransform(coefficient, pixel);
	} else {
		std::memset(pixel, 0, sizeof(pixel));
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			plane[block][i * 16 + j] =
					clamp(subblock[i][j] + pixel[i * 4 + j]);
		}
		left[i] = plane[block][i * 16 + 3];
	}

	for (int b = 1; b < 3; ++b) {
		predictSingleSubblock(subblock, above + b * 4, left,
				*(above + b * 4 - 1), above + b * 4 + 4,
				info->submode[b]);
		if (!skipCoeff) {
			decodeCoeffArray(coefficient, false, 1 + b, kFullLumaPlane,
					info);
			inverseDiscreteCosineTransform(coefficient, pixel);
		}

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				plane[block][i * 16 + b * 4 + j] =
						clamp(subblock[i][j] + pixel[i * 4 + j]);
			}
			left[i] = plane[block][i * 16 + b * 4 + 3];
		}
	}

	predictSingleSubblock(subblock, above + 12, left, *(above + 11), extra,
			info->submode[3]);
	if (!skipCoeff) {
		decodeCoeffArray(coefficient, false, 4, kFullLumaPlane, info);
		inverseDiscreteCosineTransform(coefficient, pixel);
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			plane[block][i * 16 + 12 + j] =
					clamp(subblock[i][j] + pixel[i * 4 + j]);
		}
	}
	for (int row = 1; row < 4; ++row) {
		for (int col = 0; col < 3; ++col) {
			above = &plane[block][row * 4 * 16 - 16];
			if (col == 0) {
				corner = plane[block - 1][row * 4 * 16 - 1];
			} else {
				corner = above[col * 4 - 1];
			}
			predictSingleSubblock(subblock, above + col * 4,
					left + row * 4, corner, above + col * 4 + 4,
					info->submode[row * 4 + col]);
			if (!skipCoeff) {
				decodeCoeffArray(coefficient, false, row * 4 + col + 1,
						kFullLumaPlane, info);
				inverseDiscreteCosineTransform(coefficient, pixel);
			}
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					plane[block][row * 4 * 16 + i * 16 + col * 4 + j] =
							clamp(subblock[i][j] + pixel[i * 4 + j]);
				}
				left[row * 4 + i] =
						plane[block][row * 4 * 16 + i * 16 + col * 4 + 3];
			}
		}
		predictSingleSubblock(subblock, above + 12, left + row * 4,
				*(left + row * 4 - 1), extra, info->submode[row * 4 + 3]);
		if (!skipCoeff) {
			decodeCoeffArray(coefficient, false, row * 4 + 4,
					kFullLumaPlane, info);
			inverseDiscreteCosineTransform(coefficient, pixel);
		}
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				plane[block][row * 4 * 16 + i * 16 + 12 + j] =
						clamp(subblock[i][j] + pixel[i * 4 + j]);
			}
		}
	}
}

void DecoderDriver::predictChromaBlock(FrameBuffer::Chroma* plane,
		int block) {
	int offsetAbove = block - context.horizon.blockCount - 1;
	Pixel above[8], left[8];
	std::memcpy(above, &plane[offsetAbove][8 * 7], sizeof(above));
	for (int i = 0; i < 8; ++i) {
		left[i] = plane[block - 1][8 * i + 7];
	}
	Pixel corner = plane[offsetAbove - 1][63];
	predictWholeBlock<8>(block, plane[block], above, left, corner,
			context.block[kCurrentFrame][block].chromaMode);
}

const Probability* DecoderDriver::selectCoeffProbability(int coeffArrayId,
		PlaneType plane, int position, BlockInfo* info) {
	const static int kAboveCoeff[] = {
		0,
		-13, -14, -15, -16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
		-19, -20, 17, 18, -23, -24, 21, 22
	};
	const static int kLeftCoeff[] = {
		0,
		-4, 1, 2, 3, -8, 5, 6, 7, -12, 9, 10, 11, -16, 13, 14, 15,
		-18, 17, -20, 19, -22, 21, -24, 23
	};
	int neighbor = 0;
	if (coeffArrayId == 0) {
		for (BlockInfo* cursor = info - context.horizon.blockCount - 1,
					*end = context.horizon.blockCount + 1
							+ context.block[kCurrentFrame];
				cursor > end;
				cursor -= context.horizon.blockCount + 1) {
			if (cursor->hasVirtualPlane()) {
				neighbor += cursor->hasCoeff[0];
				break;
			}
		}
		int first = (info - context.block[kCurrentFrame])
				/ (context.horizon.blockCount + 1)
				* (context.horizon.blockCount + 1);
		for (BlockInfo* end = context.block[kCurrentFrame] + first,
						*cursor = info - 1;
				cursor > end; --cursor) {
			if (cursor->hasVirtualPlane()) {
				neighbor += cursor->hasCoeff[0];
				break;
			}
		}
	} else {
		BlockInfo* cursor;
		int id = kAboveCoeff[coeffArrayId];
		if (id < 0) {
			cursor = info - context.horizon.blockCount - 1;
			id = -id;
		} else {
			cursor = info;
		}
		if (cursor->hasCoeff[id]) {
			++neighbor;
		}

		id = kLeftCoeff[coeffArrayId];
		if (id < 0) {
			cursor = info - 1;
			id = -id;
		} else {
			cursor = info;
		}
		if (cursor->hasCoeff[id]) {
			++neighbor;
		}
	}
	return context.coeff[plane][kCoeffBand[position]][neighbor];
}

void DecoderDriver::dequantizeCoefficient(short* coefficient,
		PlaneType plane) {
	static const short kDcQuantizerTable[] = {
		4,   5,   6,   7,   8,   9,  10,  10,   11,  12,  13,  14,  15,
		16,  17,  17,  18,  19,  20,  20,  21,   21,  22,  22,  23,  23,
		24,  25,  25,  26,  27,  28,  29,  30,   31,  32,  33,  34,  35,
		36,  37,  37,  38,  39,  40,  41,  42,   43,  44,  45,  46,  46,
		47,  48,  49,  50,  51,  52,  53,  54,   55,  56,  57,  58,  59,
		60,  61,  62,  63,  64,  65,  66,  67,   68,  69,  70,  71,  72,
		73,  74,  75,  76,  76,  77,  78,  79,   80,  81,  82,  83,  84,
		85,  86,  87,  88,  89,  91,  93,  95,   96,  98, 100, 101, 102,
		104, 106, 108, 110, 112, 114, 116, 118, 122, 124, 126, 128, 130,
		132, 134, 136, 138, 140, 143, 145, 148, 151, 154, 157
	};
	short factor = kDcQuantizerTable[getQuantizerIndex(plane, true)];
	if (plane == kVirtualPlane) {
		factor *= 2;
	} else if (plane == kChromaPlane && factor > 132) {
		factor = 132;
	}
	coefficient[0] *= factor;

	static const short kAcQuantizerTable[] = {
		4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
		17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
		30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
		43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  60,  62,  64,  66,  68,  70,  72,  74,  76,  78,
		80,  82,  84,  86,  88,  90,  92,  94,  96,  98, 100, 102, 104,
		106, 108, 110, 112, 114, 116, 119, 122, 125, 128, 131, 134, 137,
		140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 177,
		181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229,
		234, 239, 245, 249, 254, 259, 264, 269, 274, 279, 284,
	};
	factor = kAcQuantizerTable[getQuantizerIndex(plane, false)];
	if (plane == kVirtualPlane) {
		factor = factor * 155 / 100;
		if (factor < 8) {
			factor = 8;
		}
	}
	for (int i = 1; i < 16; ++i) {
		coefficient[i] *= factor;
	}
}

int8_t DecoderDriver::getQuantizerIndex(PlaneType plane, bool first) {
	switch (plane * 256 + first) {
	case kChromaPlane * 256 + 1:
		return context.quantizer[kChromaDc];
	case kChromaPlane * 256:
		return context.quantizer[kChromaAc];
	case kVirtualPlane * 256 + 1:
		return context.quantizer[kVirtualDc];
	case kVirtualPlane * 256:
		return context.quantizer[kVirtualAc];
	case kFullLumaPlane * 256 + 1:
	case kPartialLumaPlane * 256 + 1:
		return context.quantizer[kLumaDc];
	case kFullLumaPlane * 256:
	case kPartialLumaPlane * 256:
		return context.quantizer[kLumaAc];
	default:
		RAISE(Unpossible);
	}
}

void DecoderDriver::writeDisplayBuffer() {
	for (int i = 0; i < context.vertical.blockCount; ++i) {
		for (int j = 0; j < context.horizon.blockCount; ++j) {
			int tmp = (i + 1) * (context.horizon.blockCount + 1) + j + 1;
			FrameBuffer::Luma* luma =
					context.buffer[kCurrentFrame]->luma + tmp;
			for (int row = 0;
					row < 16 && row + i * 16 < context.vertical.length;
					++row) {
				Pixel* target = display.luma + j * 16
						+ (row + i * 16) * context.horizon.length;
				std::memcpy(target, *luma + row * 16, 16);
			}
		}
	}
	int width = context.horizon.length / 2;
	int height = context.vertical.length / 2;
	for (int p = 0; p < 2; ++p) {
		for (int i = 0; i < context.vertical.blockCount; ++i) {
			for (int j = 0; j < context.horizon.blockCount; ++j) {
				int tmp = (i + 1) * (context.horizon.blockCount + 1)
						+ j + 1;
				FrameBuffer::Chroma* chroma =
						context.buffer[kCurrentFrame]->chroma[p] + tmp;
				for (int row = 0; row < 8 && row + i * 8 < height;
						++row) {
					Pixel* target =
							display.chroma[p] + (row + i * 8) * width
							+ j * 8;
					std::memcpy(target, *chroma + row * 8, 8);
				}
			}
		}
	}
}

template<int SIZE>
void DecoderDriver::predictWholeBlock(int block, Pixel* data,
		const Pixel* above, const Pixel* left, Pixel corner, int8_t mode) {
	switch (mode) {
	case kAverageMode: {
		uint32_t sum = 0;
		int total = 0;
		if (block > (context.horizon.blockCount + 1) * 2) {
			for (int i = 0; i < SIZE; ++i) {
				sum += above[i];
			}
			sum += SIZE / 2;
			total += SIZE;
		}
		if (block % (context.horizon.blockCount + 1) != 1) {
			for (int i = 0; i < SIZE; ++i) {
				sum += left[i];
			}
			sum += SIZE / 2;
			total += SIZE;
		}
		Pixel average = (total == 0) ? 128 : sum / total;
		std::memset(data, average, SIZE * SIZE);
		break; }
	case kHorizontalMode:
		for (int i = 0; i < SIZE; ++i) {
			std::memset(data + i * SIZE, left[i], SIZE);
		}
		break;
	case kVerticalMode:
		for (int i = 0; i < SIZE; ++i) {
			std::memcpy(data + i * SIZE, above, SIZE);
		}
		break;
	case kTrueMotionMode:
		for (int i = 0; i < SIZE; ++i) {
			for (int j = 0; j < SIZE; ++j) {
				data[i * SIZE + j] = clamp(above[j] + left[i] - corner);
			}
		}
		break;
	default:
		RAISE(Unpossible);
	}
}
