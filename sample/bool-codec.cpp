#include <cstdio>
#include <cherry/decode/BoolDecoder.hpp>
#include <cherry/encode/BoolEncoder.hpp>
#include <cherry/utility/bit.hpp>

using namespace cherry;

uint8_t getProbability(char* string, int length) {
	int count = 0;
	int index = 0;
	for (; index < length - 3; index += 4) {
		count += 32 - getHammingWeight(*(int32_t*)(string + index));
	}
	int32_t tail = 0;
	std::memcpy(&tail, string + index, length - index);
	count += 32 - getHammingWeight(tail);
	return count * 32 / length; // == count * 256 / (length * 8)
}

int main(int argc, char** argv) {
	int length = 0;
	if (argc != 2 || (length = std::strlen(argv[1])) <= 0) {
		std::fprintf(stderr, "Usage: %s <string>\n", argv[0]);
		return 1;
	}
	uint8_t probability = getProbability(argv[1], length);
	std::printf("Probability: %u\n", probability);

	char buffer[256];
	BoolEncoder encoder(buffer, sizeof(buffer), probability);
	encoder.encode(argv[1], length);
	encoder.flush();
	size_t outputSize = encoder.getOutputSize();

	std::printf("Original size: %d, Compressed size: %zu\n", length,
			outputSize);

	BoolDecoder decoder(buffer, outputSize, probability);
	for (int i = 0; i < length; ++i) {
		std::fputc(decoder.decode(), stdout);
	}
	std::fputc('\n', stdout);

	return 0;
}
