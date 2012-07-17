#ifndef _CHERRY_VP8_DECODER_DRIVER_HPP_INCLUDED_
#define _CHERRY_VP8_DECODER_DRIVER_HPP_INCLUDED_

#include <stdint.h>
#include <cstring>
#include <cherry/vp8/types.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/BlockInfo.hpp>
#include <cherry/vp8/FrameBuffer.hpp>
#include <cherry/decode/BoolDecoder.hpp>

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

	Pixel* getLuma() const {
		return display.luma;
	}

	Pixel* getChroma(int i) const {
		return display.chroma[i];
	}

private:
	enum PlaneType {
		kPartialLumaPlane, kVirtualPlane, kChromaPlane, kFullLumaPlane
	};

	enum QuantizerIndex {
		kLumaAc, kLumaDc, kVirtualDc, kVirtualAc, kChromaDc, kChromaAc,
		kQuantizerCount
	};

	typedef Probability ProbabilityTable[8][3][kCoeffTokenCount - 1];

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
		} filter;
		struct CoeffContext {
			// Only 9 of the 12 booleans are used, the rest of are used
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
	struct Display {
		Pixel* luma;
		Pixel* chroma[2];

		Display() : luma(NULL) {
			chroma[0] = chroma[1] = NULL;
		}

		~Display() {
			destroy();
		}

		void create(int width, int height) {
			luma = new Pixel[width * height];
			chroma[0] = new Pixel[width / 2 * height / 2];
			chroma[1] = new Pixel[width / 2 * height / 2];
		}

		void destroy() {
			delete[] luma;
			delete[] chroma[0];
			delete[] chroma[1];
		}
	} display;

	static Pixel clamp(int value);

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

	template<int BLOCK_SIZE>
	void predictWholeBlock(int row, int column, Pixel* above, Pixel* left,
			Pixel* target, int8_t mode);
};

} // namespace cherry

#endif // _CHERRY_VP8_MAIN_DRIVER_HPP_INCLUDED_
