#include <cstdio>
#include <cstddef>
#include <cherry/decode/BoolDecoder.hpp>

using namespace cherry;

int main() {
	int data = 0xf1f2f380;
	BoolDecoder decoder;
	decoder.reload(&data, sizeof(data));
	for (size_t i = 0; i < sizeof(data); ++i) {
		std::printf("%x\n", decoder.uint<8>());
	}
	return 0;
}
