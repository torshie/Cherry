#ifndef _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_
#define _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_

#include <cherry/vp8/types.hpp>

namespace cherry {

struct FrameBuffer {
	typedef Pixel Luma[16 * 16];
	typedef Pixel Chroma[8 * 8];

	FrameBuffer(int width, int height);

	~FrameBuffer() {
		delete[] luma;
		delete[] chroma[0];
		delete[] chroma[1];
	}

	Luma* luma;
	Chroma* chroma[2];
};

} // namespace cherry

#endif // _CHERRY_VP8_FRAME_BUFFER_HPP_INCLUDED_
