#include <cstdio>
#include <cstring>
#include <cstdlib>

int main(int argc, char** argv) {
	int width, height, side;
	if (argc != 4 || std::sscanf(argv[1], "%d", &width) != 1
			|| std::sscanf(argv[2], "%d", &height) != 1
			|| std::sscanf(argv[3], "%d", &side) != 1) {
		std::fprintf(stderr, "Usage: %s <width> <height> <side>\n",
				argv[0]);
		return 1;
	}
	char* input = (char*)std::malloc(width * height);
	if (std::fread(input, 1, width * height, stdin)
			!= (size_t)width * height) {
		fprintf(stderr, "Failed to read from stdin\n");
		return 1;
	}
	char block[16 * 16];
	int countX = (width + side - 1) / side;
	int countY = (height + side - 1) / side;
	for (int i = 0; i < countY; ++i) {
		for (int j = 0; j < countX; ++j) {
			std::memset(block, 0, sizeof(block));
			for (int y = 0; y < side && y + i * side < height; ++y) {
				std::memcpy(block + y * side,
						input + i * side * width + j * side + y * width,
						side);
			}
			std::fwrite(block, 1, side * side, stdout);
		}
	}
	std::free(input);
	return 0;
}
