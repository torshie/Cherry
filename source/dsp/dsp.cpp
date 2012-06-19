#include <cherry/dsp/dsp.hpp>

namespace cherry {

// Inverse Walsh-Hadamard Transform.
//                | 1  1  1  1 |
// Let matrix H = | 1  1 -1 -1 |, input matrix I = *input*,
//                | 1 -1 -1  1 |
//                | 1 -1  1 -1 |
// then the output matrix O = (H . I . H + 3) / 8, here "+ 3" means add 3
// to every element of the matrix.
void inverseWalshHadamardTransform(const short* input, short* output) {
	const short* ip = input;
	short* op = output;
	for (int i = 0; i < 4; ++i, ++ip, ++op) {
		int a1 = ip[0] + ip[12];
		int b1 = ip[4] + ip[8];
		int c1 = ip[4] - ip[8];
		int d1 = ip[0] - ip[12];
		op[0] = a1 + b1;
		op[4] = c1 + d1;
		op[8] = a1 - b1;
		op[12]= d1 - c1;
	}

	ip = output;
	op = output;
	for (int i = 0; i < 4; ++i, ip += 4, op += 4) {
		int a1 = ip[0] + ip[3];
		int b1 = ip[1] + ip[2];
		int c1 = ip[1] - ip[2];
		int d1 = ip[0] - ip[3];

		int a2 = a1 + b1;
		int b2 = c1 + d1;
		int c2 = a1 - b1;
		int d2 = d1 - c1;

		op[0] = (a2 + 3) >> 3;
		op[1] = (b2 + 3) >> 3;
		op[2] = (c2 + 3) >> 3;
		op[3] = (d2 + 3) >> 3;
	}
}

void inverseDiscreteCosineTransform(const short* input, short* output) {
	enum {
		kCosineFactor = 20091, // = (cos(pi/8) * sqrt(2) - 1) * 65536
		kSineFactor = 35468 // = sin(pi/8) * sqrt(2) * 65536
	};
	const short* ip = input;
	short* op = output;

	for(int i = 0; i < 4 ; ++i, ++ip, ++op) {
		int a1 = ip[0] + ip[8];
		int b1 = ip[0] - ip[8];

		int temp1 = (ip[4] * kSineFactor) >> 16;
		int temp2 = ip[12] + ((ip[12] * kCosineFactor) >> 16);
		int c1 = temp1 - temp2;

		temp1 = ip[4] + ((ip[4] * kCosineFactor) >> 16);
		temp2 = (ip[12] * kSineFactor) >> 16;
		int d1 = temp1 + temp2;

		op[0] = a1 + d1;
		op[12] = a1 - d1;
		op[4] = b1 + c1;
		op[8] = b1 - c1;
	}

	ip = output;
	op = output;
	for (int i = 0; i < 4; ++i, ip += 4, op += 4) {
		int a1 = ip[0] + ip[2];
		int b1 = ip[0] - ip[2];

		int temp1 = (ip[1] * kSineFactor) >> 16;
		int temp2 = ip[3] + ((ip[3] * kCosineFactor) >> 16);
		int c1 = temp1 - temp2;

		temp1 = ip[1] + ((ip[1] * kCosineFactor) >> 16);
		temp2 = (ip[3] * kSineFactor) >> 16;
		int d1 = temp1 + temp2;

		op[0] = (a1 + d1 + 4) >> 3;
		op[3] = (a1 - d1 + 4) >> 3;
		op[1] = (b1 + c1 + 4) >> 3;
		op[2] = (b1 - c1 + 4) >> 3;
	}
}

} // namespace cherry
