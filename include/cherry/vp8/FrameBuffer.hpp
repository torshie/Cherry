#ifndef _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_
#define _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_

#include <cstddef>
#include <cherry/vp8/BlockInfo.hpp>
#include <cherry/utility/memory.hpp>

namespace cherry {

struct FrameBuffer {
	friend class DecoderDriver;

	BlockInfo* info;
	union {
		struct __attribute__((__packed__)) {
			Pixel* luma;
			Pixel* chroma[2];
		};
		Pixel* plane[3];
	};

	FrameBuffer() : info(NULL), luma(NULL) {
		chroma[0] = chroma[1] = NULL;
	}

	~FrameBuffer() {
		destroy();
	}

	void destroy() {
		release<16>(luma);
		release<8>(chroma[0]);
		release<8>(chroma[1]);
		delete[] info;
	}

	void create(int width, int height);
};

} // namespace cherry

#endif // _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_
