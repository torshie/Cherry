#include <unistd.h>
#include <cherry/unpack/IvfUnpacker.hpp>
#include <cherry/thread/Guard.hpp>
#include <cherry/wrapper/wrapper.hpp>

using namespace cherry;

IvfUnpacker::IvfUnpacker(const char* filename) {
	int fd = wrapper::open_(filename, O_RDONLY);
	struct stat s;
	wrapper::fstat_(fd, &s);
	fileSize = s.st_size;
	long pageSize = sysconf(_SC_PAGE_SIZE);
	mapSize = (fileSize + pageSize - 1) / pageSize * pageSize;
	memory = wrapper::mmap_(NULL, mapSize, PROT_READ, MAP_PRIVATE, fd, 0);

	FileHeader* header = (FileHeader*)memory;
	if (header->signature[0] != 'D' || header->signature[1] != 'I'
			|| header->signature[2] != 'V' || header->signature[3] != 'F'
			|| header->version != 0 || header->headerSize != sizeof(*header)) {
		RAISE(InvalidArgument, "File %s isn't a supported IVF file");
	}
	offset = sizeof(*header);
}

IvfUnpacker::~IvfUnpacker() {
	wrapper::munmap_(memory, mapSize);
}

const void* IvfUnpacker::getCompressedFrame(int* size) {
	if (offset >= fileSize) {
		return NULL;
	}

	FrameHeader* header;
	{
		Guard(mutex);
		if (offset >= fileSize) {
			return NULL;
		}
		header = (FrameHeader)(memory + offset);
		offset += header->frameSize + sizeof(*header);
	}
	*size = header->frameSize;
	return header + 1;
}
