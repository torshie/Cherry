#ifndef _CHERRY_UTILITY_MEMORY_HPP_INCLUDED_
#define _CHERRY_UTILITY_MEMORY_HPP_INCLUDED_

#include <stdint.h>
#include <cstdlib>

namespace cherry {

template<int ALIGNMENT>
inline void* allocate(size_t size) {
	uintptr_t location = (uintptr_t)std::malloc(size + ALIGNMENT);
	uintptr_t shift = location % ALIGNMENT;
	if (shift != 0) {
		location += ALIGNMENT - shift;
		*(int8_t*)(location - 1) = ALIGNMENT - shift;
	}
	return (void*)location;
}

template<int ALIGNMENT>
inline void release(void* pointer) {
	if ((uintptr_t)pointer % ALIGNMENT != 0) {
		pointer = (char*)pointer - *((int8_t*)pointer - 1);
	}
	std::free(pointer);
}

} // namespace cherry;

#endif // _CHERRY_UTILITY_MEMORY_HPP_INCLUDED_
