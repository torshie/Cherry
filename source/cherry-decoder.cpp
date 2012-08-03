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
	MainDecoder decoder;
	DummyDisplay display(argv[2]);
	decoder.setDisplay(&display);
	int size = 0;
	const void* frame = unpacker.getCompressedFrame(&size);
	while (frame != NULL) {
		decoder.setFrameData(frame, size);
		decoder.decodeFrame();
		frame = unpacker.getCompressedFrame(&size);
	}
	return 0;
}
