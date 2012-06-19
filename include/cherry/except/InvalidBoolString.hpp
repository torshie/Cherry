#ifndef _CHERRY_EXCEPT_INVALID_BOOL_STRING_HPP_INCLUDED_
#define _CHERRY_EXCEPT_INVALID_BOOL_STRING_HPP_INCLUDED_

#include <cstddef>
#include <cherry/except/BasicExcept.hpp>

namespace cherry {

class InvalidBoolString : public BasicExcept {
public:
	InvalidBoolString(const char* origin, const char* format = NULL, ...)
			: BasicExcept(origin) {
		BUILD_MESSAGE_BUFFER(format);
	}
};

} // namespace cherry

#endif // _CHERRY_EXCEPT_INVALID_BOOL_STRING_HPP_INCLUDED_
