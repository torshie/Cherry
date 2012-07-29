#ifndef _CHERRY_VP8_MAIN_DECODER_HPP_INCLUDED_
#define _CHERRY_VP8_MAIN_DECODER_HPP_INCLUDED_

#include <stdint.h>
#include <cstring>
#include <cherry/vp8/types.hpp>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/BlockInfo.hpp>
#include <cherry/vp8/FrameBuffer.hpp>
#include <cherry/vp8/ProbabilityManager.hpp>
#include <cherry/vp8/LoopFilter.hpp>
#include <cherry/vp8/Geometry.hpp>
#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/display/Display.hpp>

namespace cherry {

class MainDecoder {
public:
	MainDecoder();

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
	Geometry geometry;
	Display* display;
	ProbabilityManager probabilityManager;
	LoopFilter filter;

	void decodeFrameHeader();
	void decodeBlockInfo(BlockInfo* meta);
	void decodeSubmode(BlockInfo* meta);
	void decodeFrameTag(const void* data);
	void decodeBlockCoeff(short coefficient[25][16], BlockInfo* info,
			int row, int column);
	void updateSegmentation();
	void decodeQuantizerTable();
	void updateQuantizerValue();
	void update16x16Probability();
	void updateChromaProbability();
	void updateMotionVectorProbability();
	void resizeFrame(int width, int height);
	void dequantizeCoefficient(short* coefficient, PlaneType plane,
			int lastCoeff);
};

} // namespace cherry

#endif // _CHERRY_VP8_MAIN_DECODER_HPP_INCLUDED_
