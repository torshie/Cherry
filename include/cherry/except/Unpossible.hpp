#ifndef _CHERRY_EXCEPT_UNPOSSIBLE_HPP_INCLUDED_
#define _CHERRY_EXCEPT_UNPOSSIBLE_HPP_INCLUDED_

#include <cherry/except/BasicExcept.hpp>

namespace cherry {

class Unpossible : public BasicExcept {
public:
	Unpossible(const char* origin, const char* format = NULL, ...)
			: BasicExcept(origin) {
		BUILD_MESSAGE_BUFFER(format);
	}
};

} // namespace cherry

#endif // _CHERRY_EXCEPT_UNPOSSIBLE_HPP_INCLUDED_
