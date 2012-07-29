#ifndef _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_
#define _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_

#include <cstddef>
#include <cherry/vp8/BlockInfo.hpp>
#include <cherry/vp8/Geometry.hpp>
#include <cherry/vp8/LoopFilter.hpp>
#include <cherry/display/Display.hpp>
#include <cherry/utility/memory.hpp>

namespace cherry {

class FrameBuffer {
public:
	FrameBuffer() : blockInfo(NULL), luma(NULL) {
		chroma[0] = chroma[1] = NULL;
	}

	~FrameBuffer() {
		destroy();
	}

	BlockInfo* getBlockInfo(int row, int column) {
		return blockInfo + row * geometry->blockWidth + column;
	}

	void setGeometry(const Geometry* geometry);
	void predict(int row, int column, const BlockInfo* info,
			short coefficient[25][16]);
	void writeDisplay(Display* display);
	template<FilterType TYPE>
	void applyLoopFilter(int row, int column, LoopFilter* filter);

private:
	BlockInfo* blockInfo;
	union __attribute__((__packed__)) {
		struct __attribute__((__packed__)) {
			Pixel* luma;
			Pixel* chroma[2];
		};
		Pixel* plane[3];
	};
	const Geometry* geometry;

	static Pixel clamp(int value) {
		if (value < 0) {
			return 0;
		} else if (value > 255) {
			return 255;
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

	void create(int width, int height);

	void predictLumaBlock(int row, int column, const BlockInfo* info,
			short coefficient[25][16]);
	void predictBusyLuma(Pixel* above, Pixel* left, Pixel* extra,
			Pixel* target, const BlockInfo* info,
			short coefficient[25][16]);
	void predict4x4(Pixel subblock[4][4], Pixel* above,
			Pixel* left, Pixel* extra, int8_t submode);
	void predictChromaBlock(int row, int column, const BlockInfo* info,
			short coefficient[25][16]);
	template<int BLOCK_SIZE>
	void predictWholeBlock(int row, int column, Pixel* above, Pixel* left,
			Pixel* target, int8_t mode);
	void addSubblockResidual(int row, int column, Pixel* target,
			const short* residual, Pixel subblock[4][4]);

	void destroy() {
		release<16>(luma);
		release<8>(chroma[0]);
		release<8>(chroma[1]);
		delete[] blockInfo;
	}
};

template<FilterType TYPE>
inline void FrameBuffer::applyLoopFilter(int row, int column,
		LoopFilter* filter) {
	const BlockInfo* info = getBlockInfo(row, column);
	if (filter->setLevel(info)) {
		return;
	}

	const static int kSize[3] = {
		16, 8 * (TYPE != kSimpleFilter), 8 * (TYPE != kSimpleFilter)
	};
	if (column != 1) {
		for (int p = 0; p < 3; ++p) {
			int lineSize = geometry->blockWidth * kSize[p];
			for (int i = 0; i < kSize[p]; ++i) {
				Pixel* pixel = plane[p] + column * kSize[p]
						+ (row * kSize[p] + i) * lineSize;
				filter->execute<TYPE>(pixel, 1, false);
			}
		}
	}
	if (info->lumaMode == kDummyLumaMode || !info->skipCoeff) {
		for (int p = 0; p < 3; ++p) {
			int lineSize = geometry->blockWidth * kSize[p];
			for (int j = 4; j < kSize[p]; j += 4) {
				for (int i = 0; i < kSize[p]; ++i) {
					Pixel* pixel = plane[p] + j + column * kSize[p]
							+ (row * kSize[p] + i) * lineSize;
					filter->execute<TYPE>(pixel, 1, true);
				}
			}
		}
	}
	if (row != 1) {
		for (int p = 0; p < 3; ++p) {
			int lineSize = geometry->blockWidth * kSize[p];
			for (int i = 0; i < kSize[p]; ++i) {
				Pixel* pixel = plane[p] + i
						+ row * kSize[p] * lineSize
						+ column * kSize[p];
				filter->execute<TYPE>(pixel, lineSize, false);
			}
		}
	}
	if (info->lumaMode == kDummyLumaMode || !info->skipCoeff) {
		for (int p = 0; p < 3; ++p) {
			int lineSize = geometry->blockWidth * kSize[p];
			for (int j = 4; j < kSize[p]; j += 4) {
				for (int i = 0; i < kSize[p]; ++i) {
					Pixel* pixel = plane[p] + i + column * kSize[p]
							+ (row * kSize[p] + j) * lineSize;
					filter->execute<TYPE>(pixel, lineSize, true);
				}
			}
		}
	}
}

} // namespace cherry

#endif // _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_
