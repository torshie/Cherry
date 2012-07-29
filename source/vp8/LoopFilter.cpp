#include <cstdlib>
#include <cherry/utility/misc.hpp>
#include <cherry/vp8/LoopFilter.hpp>

using namespace cherry;

void LoopFilter::Parameter::init(int level, uint8_t sharpness) {
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

bool LoopFilter::setLevel(const BlockInfo* info) {
	int l = level;
	if (delta.enabled) {
		// TODO Handle inter frames.
		l = level + delta.reference[0];
		if (info->lumaMode == kDummyLumaMode) {
			l += delta.mode[0];
		}
		l = (l < 0) ? 0 : ((l > 63) ? 63 : l);
	}
	current = param + l;
	return l == 0;
}

void LoopFilter::loadInfo(BoolDecoder* source) {
	type = (FilterType)source->uint<1>();
	level = source->uint<6>();
	uint8_t tmp = source->uint<3>();
	if (sharpness != tmp) {
		sharpness = tmp;
		for (size_t i = 0; i < ELEMENT_COUNT(param); ++i) {
			param[i].init(i, sharpness);
		}
	}
	delta.enabled = source->uint<1>();
	if (delta.enabled && source->uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			if (source->uint<1>()) {
				delta.reference[i] = source->sint<7>();
			}
		}
		for (int i = 0; i < 4; ++i) {
			if (source->uint<1>()) {
				delta.mode[i] = source->sint<7>();
			}
		}
	}
}

int8_t LoopFilter::adjust(bool useOuter, const Pixel* P1,
		Pixel* P0, Pixel* Q0, const Pixel* Q1) {
	int8_t p1 = *P1 - 128;
	int8_t p0 = *P0 - 128;
	int8_t q0 = *Q0 - 128;
	int8_t q1 = *Q1 - 128;

	int8_t a = clamp((q0 - p0) * 3 + (useOuter ? clamp(p1 - q1) : 0));
	int8_t b = (clamp(a + 3)) >> 3;
	a = clamp(a + 4) >> 3;
	*Q0 = (q0 - a) + 128;
	*P0 = (p0 + b) + 128;

	return a;
}

void LoopFilter::edgeFilter(uint8_t threshold, Pixel** pixel) {
	int8_t p2 = *pixel[1] - 128, p1 = *pixel[2] - 128,
			p0 = *pixel[3] - 128, q0 = *pixel[4] - 128,
			q1 = *pixel[5] - 128, q2 = *pixel[6] - 128;
	if (!highEdgeVariance(threshold, p1, p0, q0, q1)) {
		int8_t w = clamp(clamp(p1 - q1) + 3 * (q0 - p0));
		int8_t a = clamp((27 * w + 63) >> 7);
		*pixel[4] = (q0 - a) + 128;  *pixel[3] = (p0 + a) + 128;
		a = clamp((18 * w + 63) >> 7);
		*pixel[5] = (q1 - a) + 128;  *pixel[2] = (p1 + a) + 128;
		a = clamp((9 * w + 63) >> 7);
		*pixel[6] = (q2 - a) + 128;  *pixel[1] = (p2 + a) + 128;
	} else {
		adjust(true, pixel[2], pixel[3], pixel[4], pixel[5]);
	}
}

void LoopFilter::subblockFilter(uint8_t threshold, Pixel** pixel) {
	int8_t p1 = *pixel[2] - 128, p0 = *pixel[3] - 128,
			q0 = *pixel[4] - 128, q1 = *pixel[5] - 128;
	bool hev = highEdgeVariance(threshold, p1, p0, q0, q1);
	int8_t a = adjust(hev, pixel[2], pixel[3], pixel[4], pixel[5]);
	if (!hev) {
		a = (a + 1) >> 1;
		*pixel[5] = (q1 - a) + 128;
		*pixel[2] = (p1 + a) + 128;
	}
}

bool LoopFilter::suitable(uint8_t neighbor, uint8_t edge, Pixel** pixel) {
	int8_t p3 = *pixel[0] - 128, p2 = *pixel[1] - 128,
			p1 = *pixel[2] - 128, p0 = *pixel[3] - 128,
			q0 = *pixel[4] - 128, q1 = *pixel[5] - 128,
			q2 = *pixel[6] - 128, q3 = *pixel[7] - 128;
	return  (std::abs(p0 - q0) * 2 + std::abs(p1 - q1) / 2) <= edge
			&& std::abs(p3 - p2) <= neighbor
			&& std::abs(p2 - p1) <= neighbor
			&& std::abs(p1 - p0) <= neighbor
			&& std::abs(q3 - q2) <= neighbor
			&& std::abs(q2 - q1) <= neighbor
			&& std::abs(q1 - q0) <= neighbor;
}

namespace cherry {

template<>
void LoopFilter::execute<kSimpleFilter>(Pixel* pixel, ptrdiff_t step,
		bool subblock) {
	uint8_t edge;
	if (subblock) {
		edge = current->limit.subblock;
	} else {
		edge = current->limit.edge;
	}
	if (std::abs(pixel[-step] - pixel[0]) * 2
			+ std::abs(pixel[-2 * step] - pixel[step]) / 2 <= edge) {
		adjust(true, pixel - 2 * step, pixel - step, pixel, pixel + step);
	}
}


template<>
void LoopFilter::execute<kNormalFilter>(Pixel* after, ptrdiff_t step,
		bool subblock) {
	uint8_t edge =
			subblock ? current->limit.subblock : current->limit.edge;
	Pixel * pixel[8];
	for (int i = 0; i < 8; ++i) {
		pixel[i] = after + (i - 4) * step;
	}

	if (!suitable(current->limit.neighbor, edge, pixel)) {
		return;
	}
	if (subblock) {
		subblockFilter(current->threshold, pixel);
	} else {
		edgeFilter(current->threshold, pixel);
	}
}

} // cherry
