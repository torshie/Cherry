#include <cherry/vp8/FrameBuffer.hpp>

using namespace cherry;

void FrameBuffer::create(int width, int height) {
	luma = (Pixel*)allocate<16>(width * height * 16 * 16);
	std::memset(luma + width * 16 * 15 + 16, 127,
			width * 16 - 16);
	luma[width * 16 * 15 + 15] = 128;
	for (int i = 16; i < height * 16; ++i) {
		luma[i * width * 16 + 15] = 129;
	}

	for (int p = 0; p < 2; ++p) {
		chroma[p] = (Pixel*)allocate<8>(width * height * 8 * 8);
		std::memset(chroma[p] + width * 8 * 7 + 8, 127,
				width * 8 - 8);
		for (int i = 8; i < height * 8; ++i) {
			chroma[p][i * width * 8 + 7] = 129;
		}
	}

	info = new BlockInfo[width * height];
}
