#ifndef _CHERRY_EXCEPT_INVALID_ARGUMENT_HPP_INCLUDED_
#define _CHERRY_EXCEPT_INVALID_ARGUMENT_HPP_INCLUDED_

#include <cherry/except/BasicExcept.hpp>

namespace cherry {

class InvalidArgument : public BasicExcept {
public:
	InvalidArgument(const char* origin, const char* format = NULL, ...)
			: BasicExcept(origin) {
		BUILD_MESSAGE_BUFFER(format);
	}
};

} // namespace cherry

#endif // _CHERRY_EXCEPT_INVALID_ARGUMENT_HPP_INCLUDED_
