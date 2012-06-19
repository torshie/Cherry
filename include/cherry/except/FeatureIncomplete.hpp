#ifndef _CHERRY_EXCEPT_FEATURE_INCOMPLETE_HPP_INCLUDED_
#define _CHERRY_EXCEPT_FEATURE_INCOMPLETE_HPP_INCLUDED_

#include <cherry/except/BasicExcept.hpp>

namespace cherry {

class FeatureIncomplete : public BasicExcept {
public:
	FeatureIncomplete(const char* origin, const char* format, ...)
			: BasicExcept(origin) {
		BUILD_MESSAGE_BUFFER(format);
	}
};

} // namespace cherry

#endif // _CHERRY_EXCEPT_FEATURE_INCOMPLETE_HPP_INCLUDED_
