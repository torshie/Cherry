#include <cstring>
#include <cherry/vp8/const.hpp>
#include <cherry/vp8/ProbabilityManager.hpp>
#include <cherry/decode/BoolDecoder.hpp>

using namespace cherry;

void ProbabilityManager::reset() {
	const static Probability kCoeff[4][8][3][kCoeffTokenCount - 1] = {
#include <table/probability/defaultCoefficient.i>
	};
	std::memcpy(&coeff, &kCoeff, sizeof(coeff));
}

void ProbabilityManager::loadCoeffProb(BoolDecoder* source) {
	const static Probability
	kProbability[4][8][3][kCoeffTokenCount - 1] = {
#include <table/probability/updateCoefficient.i>
	};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 8; ++j) {
			for (int k = 0; k < 3; ++k) {
				for (int l = 0; l < kCoeffTokenCount - 1; ++l) {
					if (source->decode(kProbability[i][j][k][l])) {
						coeff[i][j][k][l] = source->uint<8>();
					}
				}
			}
		}
	}
}

void ProbabilityManager::loadReferenceProb(BoolDecoder* source) {
	reference.intra = source->uint<8>();
	reference.last = source->uint<8>();
	reference.golden = source->uint<8>();
	if (source->uint<1>()) {
		for (int i = 0; i < 4; ++i) {
			source->uint<8>();
		}
	}
	if (source->uint<1>()) {
		for (int i = 0; i < 3; ++i) {
			source->uint<8>();
		}
	}
	for (int i = 0; i < 2 * 19; ++i) {
		if (source->uint<1>()) {
			source->uint<7>();
		}
	}
}
