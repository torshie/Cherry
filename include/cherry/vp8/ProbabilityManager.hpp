#ifndef _CHERRY_VP8_PROBABILITY_MANAGER_HPP_INCLUDED_
#define _CHERRY_VP8_PROBABILITY_MANAGER_HPP_INCLUDED_

#include <cherry/vp8/types.hpp>

namespace cherry {

class BoolDecoder;
class ProbabilityManager {
	friend class BoolDecoder;
public:
	ProbabilityManager() {
		reset();
	}

	void reset();
	void loadCoeff(BoolDecoder* source);

private:
	Probability coeff[4][8][3][kCoeffTokenCount - 1];
};

} // namespace cherry

#endif // _CHERRY_VP8_PROBABILITY_MANAGER_HPP_INCLUDED_
