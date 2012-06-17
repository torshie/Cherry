#ifndef _CHERRY_UNPACK_IVF_UNPACKER_HPP_INCLUDED_
#define _CHERRY_UNPACK_IVF_UNPACKER_HPP_INCLUDED_

#include <stdint.h>
#include <cstring>
#include <cherry/unpack/Unpacker.hpp>
#include <cherry/thread/Mutex.hpp>

namespace cherry {

class IvfUnpacker : public Unpacker {
public:
	IvfUnpacker(const char* filename);
	virtual ~IvfUnpacker();

	virtual const void* getCompressedFrame(int* size);

	virtual int getCompressedFrame(void* data, int room) {
		int size;
		const void* src = getCompressedFrame(&size);
		if (src == NULL || room < size) {
			return 0;
		}
		std::memcpy(data, src, size);
		return size;
	}

private:
	struct __attribute__((__packed__)) FileHeader {
		char signature[4];
		int16_t version;
		uint16_t headerSize;
		char codec[4];
		uint16_t width;
		uint16_t height;
		uint32_t frameRate;
		uint32_t timeScale;
		uint32_t frameCount;
		uint32_t _unused;
	};

	struct __attribute__((__packed__)) FrameHeader {
		uint32_t frameSize;
		uint64_t timestamp;
	};

	unsigned char* memory;
	uint64_t mapSize;
	uint64_t fileSize;
	uint64_t offset;
	Mutex mutex;
};

} // namespace cherry

#endif // _CHERRY_UNPACK_IVF_UNPACKER_HPP_INCLUDED_
