#ifndef _CHERRY_EXCEPT_INVALID_INPUT_STREAM_HPP_INCLUDED_
#define _CHERRY_EXCEPT_INVALID_INPUT_STREAM_HPP_INCLUDED_

#include <cstddef>
#include <cherry/except/BasicExcept.hpp>

namespace cherry {

class InvalidInputStream : public BasicExcept {
public:
	InvalidInputStream(const char* origin, const char* format = NULL, ...)
			: BasicExcept(origin) {
		BUILD_MESSAGE_BUFFER(format);
	}
};

} // namespace cherry

#endif // _CHERRY_EXCEPT_INVALID_INPUT_STREAM_HPP_INCLUDED_
