#include <cstdlib>
#include <cherry/utility/misc.hpp>
#include <cherry/utility/memory.hpp>
#include <cherry/vp8/MainDecoder.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/except/InvalidInputStream.hpp>
#include <cherry/except/Unpossible.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/dsp/dsp.hpp>

using namespace cherry;

MainDecoder::MainDecoder() : frameData(NULL), frameSize(0), display(NULL) {
	std::memset(&context, 0, sizeof(context));
	std::memset(&geometry, 0, sizeof(geometry));
	for (size_t i = 0; i < ELEMENT_COUNT(context.quantizer.index); ++i) {
		context.quantizer.index[i] = -1;
	}
}

void MainDecoder::decodeFrame() {
	decodeFrameHeader();

	for (int i = 1; i < geometry.blockHeight; ++i) {
		for (int j = 1; j < geometry.blockWidth; ++j) {
			decodeBlockInfo(buffer.current.getBlockInfo(i, j));
		}
	}

	int tagSize = context.keyFrame ? 10 : 3;
	source.reload((char*)frameData + context.firstPartSize + tagSize,
			frameSize - context.firstPartSize - tagSize);

	for (int i = 1; i < geometry.blockHeight; ++i) {
		for (int j = 1; j < geometry.blockWidth; ++j) {
			BlockInfo* info = buffer.current.getBlockInfo(i, j);
			short coefficient[25][16];
			decodeBlockCoeff(coefficient, info, i, j);
			buffer.current.predict(i, j, info, coefficient);
		}
	}

	if (filter.getType() == kSimpleFilter) {
		for (int i = 1; i < geometry.blockHeight; ++i) {
			for (int j = 1; j < geometry.blockWidth; ++j) {
				buffer.current.applyLoopFilter<kSimpleFilter>(i, j,
						&filter);
			}
		}
	} else {
		for (int i = 1; i < geometry.blockHeight; ++i) {
			for (int j = 1; j < geometry.blockWidth; ++j) {
				buffer.current.applyLoopFilter<kNormalFilter>(i, j,
						&filter);
			}
		}
	}

	buffer.current.writeDisplay(display);
}

void MainDecoder::setFrameData(const void* data, size_t size) {
	frameData = data;
	frameSize = size;

	decodeFrameTag(data);
	int tagSize = context.keyFrame ? 10 : 3;
	source.reload((const char*)data + tagSize, size - tagSize);
}

void MainDecoder::decodeFrameHeader() {
	if (context.keyFrame) {
		if (source.uint<1>() != 0) {
			RAISE(InvalidInputStream, "Color space other than YUV isn't "
					"supported.");
		}
		context.disableClamping = source.uint<1>();
	}
	updateSegmentation();
	filter.loadInfo(&source);
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
	probabilityManager.loadCoeff(&source);
	context.skipping.enabled = source.uint<1>();
	if (context.skipping.enabled) {
		context.skipping.probability = source.uint<8>();
	}
	if (!context.keyFrame) {
		// TODO: Complete this branch
	}
}

void MainDecoder::decodeBlockInfo(BlockInfo* info) {
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

void MainDecoder::decodeSubmode(BlockInfo* info) {
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

void MainDecoder::decodeFrameTag(const void* data) {
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

void MainDecoder::decodeBlockCoeff(short coefficient[25][16],
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
	int total = 0;
	bool acOnly = false;
	PlaneType lumaType = kFullLumaPlane;
	if (info->hasVirtualPlane()) {
		int ctx = context.hasCoeff.above[column][8]
				+ context.hasCoeff.left[row][8];
		info->lastCoeff[24] = source.decodeCoeff(coefficient[24], false,
				kVirtualPlane, ctx, &probabilityManager);
		dequantizeCoefficient(coefficient[24], kVirtualPlane,
				info->lastCoeff[24]);
		context.hasCoeff.above[column][8] =
				context.hasCoeff.left[row][8] =
						(info->lastCoeff[24] > 0);
		acOnly = true;
		lumaType = kPartialLumaPlane;
		total += info->lastCoeff[24];
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			int ctx = context.hasCoeff.above[column][j]
					+ context.hasCoeff.left[row][i];
			int id = i * 4 + j;
			info->lastCoeff[id] = source.decodeCoeff(coefficient[id],
					acOnly, lumaType, ctx, &probabilityManager);
			dequantizeCoefficient(coefficient[id], lumaType,
					info->lastCoeff[id]);
			context.hasCoeff.above[column][j] =
					context.hasCoeff.left[row][i] =
							(info->lastCoeff[id] > 0);
			total += info->lastCoeff[id];
		}
	}
	for (int p = 0; p < 2; ++p) {
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 2; ++j) {
				int ctx = context.hasCoeff.above[column][4 + p * 2 + j]
						+ context.hasCoeff.left[row][4 + p * 2 + i];
				int id = 16 + 4 * p + 2 * i + j;
				info->lastCoeff[id] = source.decodeCoeff(coefficient[id],
						false, kChromaPlane, ctx, &probabilityManager);
				dequantizeCoefficient(coefficient[id], kChromaPlane,
						info->lastCoeff[id]);
				context.hasCoeff.above[column][4 + p * 2 + j] =
						context.hasCoeff.left[row][4 + p * 2 + i] =
								(info->lastCoeff[id] > 0);
				total += info->lastCoeff[id];
			}
		}
	}
	if (total == 0) {
		info->skipCoeff = true;
	}
}

void MainDecoder::updateSegmentation() {
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

void MainDecoder::decodeQuantizerTable() {
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

void MainDecoder::updateQuantizerValue() {
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

void MainDecoder::update16x16Probability() {
	if (source.uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			source.uint<8>();
		}
	}
}

void MainDecoder::updateChromaProbability() {
	if (source.uint<1>()) {
		for (int i = 0; i < 3; ++i) {
			source.uint<8>();
		}
	}
}

void MainDecoder::updateMotionVectorProbability() {
	for (int i = 0; i < 2 * 19; ++i) {
		if (source.uint<1>()) {
			source.uint<7>();
		}
	}
}

void MainDecoder::resizeFrame(int width, int height) {
	geometry.set(width, height);
	display->resize(width, height);
	buffer.current.setGeometry(&geometry);
	context.hasCoeff.destroy();
	context.hasCoeff.create(geometry.blockWidth, geometry.blockHeight);
}

void MainDecoder::dequantizeCoefficient(short* coefficient,
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
