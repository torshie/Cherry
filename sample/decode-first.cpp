#include <cstdio>
#include <cherry/vp8/DecoderDriver.hpp>
#include <cherry/unpack/IvfUnpacker.hpp>
#include <cherry/display/DummyDisplay.hpp>
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
	DummyDisplay display(argv[2]);
	driver.setDisplay(&display);
	driver.setFrameData(frame, size);
	driver.decodeFrame();
	return 0;
}
