#ifndef _CHERRY_UTILITY_MEMORY_HPP_INCLUDED_
#define _CHERRY_UTILITY_MEMORY_HPP_INCLUDED_

#include <stdint.h>
#include <cstdlib>

namespace cherry {

template<int ALIGNMENT>
inline void* allocate(size_t size) {
	uint8_t* pointer = (uint8_t*)std::malloc(size + ALIGNMENT * 2);
	uint8_t shift = ALIGNMENT;
	if ((uintptr_t)pointer % ALIGNMENT != 0) {
		shift += ALIGNMENT - (uintptr_t)pointer % ALIGNMENT;
	}
	pointer += shift;
	pointer[-ALIGNMENT] = shift;
	return (void*)pointer;
}

template<int ALIGNMENT>
inline void release(void* pointer) {
	if (pointer != NULL) {
		pointer = (uint8_t*)pointer - *((uint8_t*)pointer - ALIGNMENT);
	}
	std::free(pointer);
}

} // namespace cherry;

#endif // _CHERRY_UTILITY_MEMORY_HPP_INCLUDED_
