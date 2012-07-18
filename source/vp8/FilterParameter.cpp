#include <cherry/vp8/FilterParameter.hpp>

using namespace cherry;

void FilterParameter::init(int level, uint8_t sharpness) {
	limit.neighbor = level;
	if (sharpness != 0) {
		limit.neighbor >>= sharpness > 4 ? 2 : 1;
		if (limit.neighbor > 9 - sharpness) {
			limit.neighbor = 9 - sharpness;
		}
	}
	if (limit.neighbor == 0) {
		limit.neighbor = 1;
	}
	limit.edge = (level + 2) * 2 + limit.neighbor;
	limit.subblock = level * 2 + limit.neighbor;
	threshold = 0;

	// TODO Handle inter frames.
	if (level >= 40) {
		threshold = 2;
	} else if (level >= 15) {
		threshold = 1;
	}
}
