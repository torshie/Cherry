#ifndef _CHERRY_VP8_DECODER_DRIVER_HPP_INCLUDED_
#define _CHERRY_VP8_DECODER_DRIVER_HPP_INCLUDED_

#include <cstddef>
#include <cstring>
#include <cherry/vp8/types.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/FrameBuffer.hpp>
#include <cherry/utility/misc.hpp>
#include <cherry/decode/BoolDecoder.hpp>

namespace cherry {

class DecoderDriver {
public:
	DecoderDriver() : source(NULL), frameData(NULL), frameSize(0) {
		std::memset(&context, 0, sizeof(context));
		std::memset(&display, 0, sizeof(display));
		std::memcpy(&context.coeff, &kDefaultCoeff, sizeof(kDefaultCoeff));
	}

	~DecoderDriver() {
		for (size_t i = 0; i < ELEMENT_COUNT(context.block); ++i) {
			delete[] context.block[i];
		}
		for (size_t i = 0; i < ELEMENT_COUNT(context.buffer); ++i) {
			delete context.buffer[i];
		}
		delete[] display.luma;
		delete[] display.chroma[0];
		delete[] display.chroma[1];
		delete source;
	}

	void decodeFrame();
	void setFrameData(const void* data, size_t size);

	Pixel* getChroma(int plane) const {
		return display.chroma[plane];
	}

	Pixel* getLuma() const {
		return display.luma;
	}

	void getImageSize(int* width, int* height) const {
		*width = context.horizon.length;
		*height = context.vertical.length;
	}

private:
	enum FrameType {
		kCurrentFrame, kGoldenFrame, kPreviousFrame, kAlternateFrame,
	};

	enum PlaneType {
		kPartialLumaPlane, kVirtualPlane, kChromaPlane, kFullLumaPlane
	};

	struct BlockInfo {
		bool skipCoeff;
		bool hasCoeff[25];
		int8_t lumaMode;
		int8_t submode[16];
		int8_t chromaMode;
		int8_t segment;

		BlockInfo() {
			for (size_t i = 0; i < ELEMENT_COUNT(hasCoeff); ++i) {
				hasCoeff[i] = false;
			}
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
	};

	BoolDecoder* source;
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
			uint8_t scale;
			uint16_t length;
			uint16_t blockCount;
		} horizon, vertical;
		struct {
			bool enabled;
			bool updateMapping;
			Probability probability[kSegmentCount - 1];
			int8_t quantizer[kSegmentCount];
			int8_t loopFilter[kSegmentCount];
		} segment;
		int8_t quantizer[kQuantizerCount];
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
		} filter;
		BlockInfo* block[4];
		FrameBuffer* buffer[4];
	} context;
	struct {
		Pixel* luma;
		Pixel* chroma[2];
	} display;

	static int8_t getDummySubmode(int8_t intraMode);
	static Pixel clamp(int value);
	static void predictSingleSubblock(Pixel subblock[4][4],
			Pixel* above, Pixel* left, Pixel coner, Pixel* extra,
			int8_t submode);

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
	short decodeSingleCoeff(const TreeIndex* tree,
			const Probability* probability);
	short decodeTokenOffset(int token);
	void decodeFrameTag(const void* data);
	void addChromaResidual(int block);
	void decodeCoeffArray(short* coefficient, bool acOnly,
			int coeffArrayId, PlaneType plane, BlockInfo* meta);
	void updateLoopFilter();
	void updateSegmentation();
	void updateQuantizerTable();
	void updateCoeffProbability();
	void update16x16Probability();
	void updateChromaProbability();
	void updateMotionVectorProbability();
	void resizeFrame(int width, int height);
	void buildMacroblock(int block);
	void decodeLumaBlock(FrameBuffer::Luma* plane, int block);
	void predictSubblock(FrameBuffer::Luma* plane, int block, Pixel* above,
			Pixel* left, Pixel corner, Pixel* extra);
	void predictChromaBlock(FrameBuffer::Chroma* plane, int block);
	const Probability* selectCoeffProbability(int coeffArrayId,
			PlaneType plane, int position, BlockInfo* meta);
	void dequantizeCoefficient(short* coefficient, PlaneType plane);
	int8_t getQuantizerIndex(PlaneType plane, bool first);
	void writeDisplayBuffer();

	template<int SIZE>
	void predictWholeBlock(int block, Pixel* data, const Pixel* above,
			const Pixel* left, Pixel corner, int8_t mode);
};

} // namespace cherry

#endif // _CHERRY_VP8_MAIN_DRIVER_HPP_INCLUDED_
