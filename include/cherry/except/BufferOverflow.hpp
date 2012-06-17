#ifndef _CHERRY_EXCEPT_BUFFER_OVERFLOW_HPP_INCLUDED_
#define _CHERRY_EXCEPT_BUFFER_OVERFLOW_HPP_INCLUDED_

#include <cstring>
#include <cherry/except/BasicExcept.hpp>

namespace cherry {

class BufferOverflow : public BasicExcept {
public:
	BufferOverflow(const char* origin, const char* format = NULL, ...)
			: BasicExcept(origin) {
		BUILD_MESSAGE_BUFFER(format);
	}
};

} // namespace cherry

#endif // _CHERRY_EXCEPT_BUFFER_OVERFLOW_HPP_INCLUDED_
