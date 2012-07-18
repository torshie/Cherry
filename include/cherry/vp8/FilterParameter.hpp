#ifndef _CHERRY_VP8_FILTER_PARAMETER_HPP_INCLUDED_
#define _CHERRY_VP8_FILTER_PARAMETER_HPP_INCLUDED_

#include <stdint.h>

namespace cherry {

struct FilterParameter {
	uint8_t threshold;
	struct {
		uint8_t edge;
		uint8_t subblock;
		uint8_t neighbor;
	} limit;

	void init(int level, uint8_t sharpness);
};

} // namespace cherry

#endif // _CHERRY_VP8_FILTER_PARAMETER_HPP_INCLUDED_
