#include <unistd.h>
#include <cherry/unpack/IvfUnpacker.hpp>
#include <cherry/wrapper/wrapper.hpp>
#include <cherry/except/InvalidArgument.hpp>

using namespace cherry;

IvfUnpacker::IvfUnpacker(const char* filename) {
	mapIvfFile(filename);
	FileHeader* header = (FileHeader*)memory;
	if (header->signature[0] != 'D' || header->signature[1] != 'K'
			|| header->signature[2] != 'I' || header->signature[3] != 'F'
			|| header->version != 0
			|| header->headerSize != sizeof(*header)) {
		RAISE(InvalidArgument, "File %s isn't a supported IVF file",
				filename);
	}
	offset = sizeof(*header);
}

const void* IvfUnpacker::getCompressedFrame(int* size) {
	if (offset >= fileSize) {
		return NULL;
	}
	FrameHeader* header = (FrameHeader*)(memory + offset);
	offset += header->frameSize + sizeof(*header);
	*size = header->frameSize;
	return header + 1;
}

void IvfUnpacker::mapIvfFile(const char* filename) {
	int fd = wrapper::open_(filename, O_RDONLY);
	struct stat s;
	wrapper::fstat_(fd, &s);
	fileSize = s.st_size;
	memory = (unsigned char*)wrapper::mmap_(NULL, getMapSize(), PROT_READ,
			MAP_PRIVATE, fd, 0);
	wrapper::close_(fd);
}
