#include <cstring>
#include <cherry/vp8/FrameBuffer.hpp>

using namespace cherry;

FrameBuffer::FrameBuffer(int width, int height) {
	int blockCount = (width + 1) * (height + 1);
	luma = new Luma[blockCount];
	chroma[0] = new Chroma[blockCount];
	chroma[1] = new Chroma[blockCount];

	std::memset(luma, 127, sizeof(Luma) * (width + 1));
	for (int i = 1; i < height + 1; ++i) {
		std::memset(luma[i * (width + 1)], 129, sizeof(Luma));
	}

	for (int i = 0; i < 2; ++i) {
		std::memset(chroma[i], 127, sizeof(Chroma) * (width + 1));
		for (int j = 0; j < height + 1; ++j) {
			std::memset(chroma[i][j * (width + 1)], 129, sizeof(Chroma));
		}
	}
}
