#ifndef _CHERRY_VP8_GEOMETRY_HPP_INCLUDED_
#define _CHERRY_VP8_GEOMETRY_HPP_INCLUDED_

namespace cherry {

struct Geometry {
	int displayWidth;
	int displayHeight;
	int lumaWidth;
	int lumaHeight;
	int blockWidth;
	int blockHeight;
	int chromaWidth;
	int chromaHeight;

	void set(int width, int height) {
		displayWidth = width;
		displayHeight = height;
		blockWidth = (width + 16 - 1) / 16 + 1;
		lumaWidth = blockWidth * 16;
		blockHeight = (height + 16 - 1) / 16 + 1;
		lumaHeight = blockHeight * 16;
		chromaWidth = lumaWidth / 2;
		chromaHeight = lumaHeight / 2;
	}
};

} // namespace cherry

#endif // _CHERRY_VP8_GEOMETRY_HPP_INCLUDED_
