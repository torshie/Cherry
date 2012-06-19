#include <cstdio>
#include <cstring>
#include <cstdlib>

int main(int argc, char** argv) {
	int width, height;
	if (argc !=3 || std::sscanf(argv[1], "%d", &width) != 1
			|| std::sscanf(argv[2], "%d", &height) != 1) {
		std::fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
		return 1;
	}
	char* input = (char*)std::malloc(width * height);
	if (std::fread(input, 1, width * height, stdin)
			!= (size_t)width * height) {
		fprintf(stderr, "Failed to read from stdin\n");
		return 1;
	}
	char block[16 * 16];
	int countX = (width + 15) / 16;
	int countY = (height + 15) / 16;
	for (int i = 0; i < countY; ++i) {
		for (int j = 0; j < countX; ++j) {
			std::memset(block, 0, sizeof(block));
			for (int y = 0; y < 16 && y + i * 16 < height; ++y) {
				std::memcpy(block + y * 16,
						input + i * 16 * width + j * 16 + y * width,
						16);
			}
			std::fwrite(block, 1, sizeof(block), stdout);
		}
	}
	std::free(input);
	return 0;
}
