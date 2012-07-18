#include <cherry/display/DummyDisplay.hpp>
#include <cherry/wrapper/wrapper.hpp>
#include <cherry/except/FeatureIncomplete.hpp>
#include <cherry/except/BufferOverflow.hpp>
#include <cherry/utility/misc.hpp>

using namespace cherry;

DummyDisplay::DummyDisplay(const char* filename) : width(0), height(0) {
	buffer[0] = buffer[1] = buffer[2] = NULL;
	size[0] = size[1] = size[2] = 0;
	offset[0] = offset[1] = offset[2] = 0;
	output = wrapper::fopen_(filename, "wb");
}

DummyDisplay::~DummyDisplay() {
	for (size_t i = 0; i < ELEMENT_COUNT(buffer); ++i) {
		delete[] buffer[i];
	}
	wrapper::fclose_(output);
}

void DummyDisplay::resize(int width, int height) {
	if (this->width != 0 && this->height != 0) {
		RAISE(FeatureIncomplete, "Dummy display can only be resized once");
	}
	this->width = width;
	this->height = height;
	size[0] = width * height;
	size[1] = size[2] = width / 2 * height / 2;
	for (size_t i = 0; i < ELEMENT_COUNT(buffer); ++i) {
		delete[] buffer[i];
		buffer[i] = new Pixel[size[i]];
		offset[i] = 0;
	}
	std::fprintf(output, "YUV4MPEG2 C420jpeg W%d H%d F30:1 Ip\n", width,
			height);
}

size_t DummyDisplay::write(int id, const void* data, size_t length) {
	if (length + offset[id] > size[id]) {
		RAISE(BufferOverflow, "Display buffer is full");
	}
	std::memcpy(buffer[id] + offset[id], data, length);
	offset[id] += length;
	return length;
}

void DummyDisplay::flush() {
	std::fputs("FRAME\n", output);
	for (size_t i = 0; i < ELEMENT_COUNT(buffer); ++i) {
		wrapper::fwrite_(buffer[i], 1, size[i], output);
	}
	wrapper::fflush_(output);
}
