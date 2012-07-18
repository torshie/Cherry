#ifndef _CHERRY_VP8_DECODER_DRIVER_HPP_INCLUDED_
#define _CHERRY_VP8_DECODER_DRIVER_HPP_INCLUDED_

#include <stdint.h>
#include <cstring>
#include <cherry/vp8/types.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/BlockInfo.hpp>
#include <cherry/vp8/FrameBuffer.hpp>
#include <cherry/vp8/FilterParameter.hpp>
#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/display/Display.hpp>

namespace cherry {

class DecoderDriver {
public:
	DecoderDriver();

	void decodeFrame();
	void setFrameData(const void* data, size_t size);

	void getImageSize(int* width, int* height) const {
		*width = geometry.displayWidth;
		*height = geometry.displayHeight;
	}

	void setDisplay(Display* display) {
		this->display = display;
	}

private:
	enum PlaneType {
		kPartialLumaPlane, kVirtualPlane, kChromaPlane, kFullLumaPlane
	};

	enum QuantizerIndex {
		kLumaAc, kLumaDc, kVirtualDc, kVirtualAc, kChromaDc, kChromaAc,
		kQuantizerCount
	};

	enum LoopFilterType {
		kNormalLoopFilter, kSimpleLoopFilter
	};

	typedef Probability ProbabilityTable[8][3][kCoeffTokenCount - 1];
	template<int FILTER_TYPE> struct LoopFilter;

	BoolDecoder source;
	const void* frameData;
	size_t frameSize;
	struct {
		bool keyFrame;
		bool invisible;
		bool disableClamping;
		bool refreshProbability;
		uint8_t version;
		uint8_t log2PartitionCount;
		uint32_t firstPartSize;
		struct {
			bool enabled;
			Probability probability;
		} skipping;
		struct {
			bool enabled;
			bool updateMapping;
			Probability probability[kSegmentCount - 1];
			int8_t quantizer[kSegmentCount];
			int8_t loopFilter[kSegmentCount];
		} segment;
		struct {
			int index[kQuantizerCount];
			short value[kQuantizerCount / 2][2];
		} quantizer;
		Probability coeff[4][8][3][kCoeffTokenCount - 1];
		struct {
			uint8_t type;
			uint8_t level;
			uint8_t sharpness;
			struct {
				bool enabled;
				int8_t reference[4];
				int8_t mode[4];
			} delta;
			FilterParameter param[64];
		} filter;
		struct CoeffContext {
			// Only 9 of the 12 booleans are used, the rest are used
			// for padding.
			bool (*above)[12];
			bool (*left)[12];

			~CoeffContext() {
				destroy();
			}

			void destroy() {
				delete[] above;
				delete[] left;
			}

			void create(int width, int height) {
				above = new bool[width][12]();
				left = new bool[height][12]();
			}
		} hasCoeff;
	} context;
	struct {
		FrameBuffer current;
	} buffer;
	struct {
		int displayWidth;
		int displayHeight;
		int lumaWidth;
		int lumaHeight;
		int blockWidth;
		int blockHeight;
		int chromaWidth;
		int chromaHeight;
	} geometry;
	Display* display;

	static int8_t commonAdjust(bool useOuter, const Pixel* P1, Pixel* P0,
			Pixel* Q0, const Pixel* Q1);
	static void simpleFilter(uint8_t edge, const Pixel* p1,
			Pixel* p0, Pixel* q0, const Pixel* q1);
	static bool normalFilterSuitable(uint8_t neighbor, uint8_t edge,
			int8_t p3, int8_t p2, int8_t p1, int8_t p0, int8_t q0,
			int8_t q1, int8_t q2, int8_t q3);
	static bool highEdgeVariance(uint8_t threshold, int8_t p1, int8_t p0,
			int8_t q0, int8_t q1);
	static void subblockFilter(uint8_t threshold, uint8_t neighbor,
			uint8_t edge, Pixel* P3, Pixel* P2, Pixel* P1, Pixel* P0,
			Pixel* Q0, Pixel* Q1, Pixel* Q2, Pixel* Q3);
	static void normalFilter(uint8_t threshold, uint8_t neighbor,
			uint8_t edge, Pixel* P3, Pixel* P2, Pixel* P1, Pixel* P0,
			Pixel* Q0, Pixel* Q1, Pixel* Q2, Pixel* Q3);

	static Pixel clamp(int value) {
		if (value < 0) {
			return 0;
		} else if (value > 255) {
			return 255;
		} else {
			return value;
		}
	}

	static int8_t signedClamp(int value) {
		if (value < -128) {
			return -128;
		} else if (value > 127) {
			return 127;
		} else {
			return value;
		}
	}

	static Pixel avg3(Pixel a, Pixel b, Pixel c) {
		return (a + b + b + c + 2) / 4;
	}

	static Pixel avg3p(const Pixel* p) {
		return avg3(p[-1], p[0], p[1]);
	}

	static Pixel avg2(Pixel a, Pixel b) {
		return (a + b + 1) / 2;
	}

	static Pixel avg2p(const Pixel* p) {
		return avg2(p[0], p[1]);
	}

	void decodeFrameHeader();
	void decodeBlockHeader(BlockInfo* meta);
	void decodeSubmode(BlockInfo* meta);
	short decodeTokenOffset(int token);
	void decodeFrameTag(const void* data);
	void addSubblockResidual(int row, int column, Pixel* target,
			const short* residual, Pixel subblock[4][4]);
	int decodeCoeffArray(short* coefficient, bool acOnly, PlaneType plane,
			int ctx);
	int decodeLargeCoeff(const Probability* probability);
	void decodeBlockCoeff(short coefficient[25][16], BlockInfo* info,
			int row, int column);
	void updateLoopFilter();
	void updateFilterParameterTable(uint8_t sharpness);
	void updateSegmentation();
	void decodeQuantizerTable();
	void updateQuantizerValue();
	void updateCoeffProbability();
	void update16x16Probability();
	void updateChromaProbability();
	void updateMotionVectorProbability();
	void resizeFrame(int width, int height);
	void decodeMacroblock(int row, int column);
	void predictLumaBlock(int row, int column, BlockInfo* info,
			short coefficient[25][16]);
	void predictBusyLuma(Pixel* above, Pixel* left, Pixel* extra,
			Pixel* target, BlockInfo* info, short coefficient[25][16]);
	void predict4x4(Pixel subblock[4][4], Pixel* above,
			Pixel* left, Pixel* extra, int8_t submode);
	void predictChromaBlock(int row, int column, BlockInfo* info,
			short coefficient[25][16]);
	void dequantizeCoefficient(short* coefficient, PlaneType plane,
			int lastCoeff);
	void writeDisplayBuffer();
	int getFilterLevel(BlockInfo* info);

	template<int FILTER_TYPE>
	void applyLoopFilter(int row, int column);
	template<int BLOCK_SIZE>
	void predictWholeBlock(int row, int column, Pixel* above, Pixel* left,
			Pixel* target, int8_t mode);
};

} // namespace cherry

#endif // _CHERRY_VP8_MAIN_DRIVER_HPP_INCLUDED_
