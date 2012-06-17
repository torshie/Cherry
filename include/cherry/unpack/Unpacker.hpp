#ifndef _CHERRY_UNPACK_UNPACKER_HPP_INCLUDED_
#define _CHERRY_UNPACK_UNPACKER_HPP_INCLUDED_

#include <cstddef>

namespace cherry {

class Unpacker {
public:
	virtual ~Unpacker() {}

	virtual int getCompressedFrame(void* buffer, int size) = 0;
	virtual const void* getCompressedFrame(int* /* size */) { return NULL; }
};

} // namespace cherry

#endif // _CHERRY_UNPACK_UNPACKER_HPP_INCLUDED_
