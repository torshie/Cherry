#include <cstdio>
#include <cherry/vp8/DecoderDriver.hpp>
#include <cherry/utility/BoolString.hpp>
#include <cherry/unpack/IvfUnpacker.hpp>
#include <cherry/wrapper/wrapper.hpp>

using namespace cherry;

int main(int argc, char** argv) {
	if (argc != 3) {
		std::fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
		return 1;
	}
	IvfUnpacker unpacker(argv[1]);
	int size = 0;
	const void* frame = unpacker.getCompressedFrame(&size);
	if (frame == NULL) {
		std::fprintf(stderr, "Failed to get compressed frame data\n");
		return 1;
	}
	DecoderDriver driver;
	driver.setFrameData(frame, size);
	driver.decodeFrame();

	FILE* output = wrapper::fopen_(argv[2], "wb");
	int width, height;
	driver.getImageSize(&width, &height);
	std::fprintf(output, "YUV4MPEG2 C420jpeg W%d H%d F30:1 Ip\nFRAME\n",
			width, height);
	wrapper::fwrite_(driver.getLuma(), 1, width * height, output);
	wrapper::fwrite_(driver.getChroma(0), 1, width / 2 * height / 2,
			output);
	wrapper::fwrite_(driver.getChroma(1), 1, width / 2 * height / 2,
			output);
	wrapper::fclose_(output);
	return 0;
}
