#include <cstdio>
#include <cherry/vp8/MainDecoder.hpp>
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
	MainDecoder decoder;
	DummyDisplay display(argv[2]);
	decoder.setDisplay(&display);
	decoder.setFrameData(frame, size);
	decoder.decodeFrame();
	return 0;
}
