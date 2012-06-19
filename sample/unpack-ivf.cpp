#include <cstdio>
#include <cherry/unpack/IvfUnpacker.hpp>
#include <cherry/wrapper/wrapper.hpp>

using namespace cherry;

int main(int argc, char** argv) {
	if (argc != 2) {
		std::fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}
	IvfUnpacker unpacker(argv[1]);
	int frameSize;
	const void* frame = unpacker.getCompressedFrame(&frameSize);
	if (frame != NULL) {
		wrapper::fwrite_(frame, 1, frameSize, stdout);
	}
	return 0;
}
