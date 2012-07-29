#include <cherry/vp8/FrameBuffer.hpp>
#include <cherry/except/Unpossible.hpp>
#include <cherry/dsp/dsp.hpp>

using namespace cherry;

void FrameBuffer::create(int width, int height) {
	luma = (Pixel*)allocate<16>(width * height * 16 * 16);
	std::memset(luma + width * 16 * 15 + 16, 127,
			width * 16 - 16);
	luma[width * 16 * 15 + 15] = 128;
	for (int i = 16; i < height * 16; ++i) {
		luma[i * width * 16 + 15] = 129;
	}

	for (int p = 0; p < 2; ++p) {
		chroma[p] = (Pixel*)allocate<8>(width * height * 8 * 8);
		std::memset(chroma[p] + width * 8 * 7 + 8, 127,
				width * 8 - 8);
		for (int i = 8; i < height * 8; ++i) {
			chroma[p][i * width * 8 + 7] = 129;
		}
	}

	blockInfo = new BlockInfo[width * height];
}

void FrameBuffer::setGeometry(const Geometry* geometry) {
	this->geometry = geometry;
	destroy();
	create(geometry->blockWidth, geometry->blockHeight);
	for (int i = 0; i < geometry->blockWidth; ++i) {
		blockInfo[i].initForPadding();
	}
	for (int i = 1; i < geometry->blockHeight; ++i) {
		blockInfo[i * geometry->blockWidth].initForPadding();
	}
}

void FrameBuffer::predict(int row, int column,
		const BlockInfo* info, short coefficient[25][16]) {
	predictLumaBlock(row, column, info, coefficient);
	predictChromaBlock(row, column, info, coefficient);
}

void FrameBuffer::predictLumaBlock(int row, int column,
		const BlockInfo* info, short coefficient[25][16]) {
	Pixel* above = luma + column * 16
			+ geometry->lumaWidth * (row * 16 - 1);
	Pixel* target = above + geometry->lumaWidth;
	Pixel* left = target - 1;
	if (info->lumaMode == kDummyLumaMode) {
		if (column == geometry->blockWidth - 1) {
			Pixel extra[4];
			std::memset(extra, above[15], sizeof(extra));
			predictBusyLuma(above, left, extra, target, info, coefficient);
		} else {
			predictBusyLuma(above, left, above + 16, target, info,
					coefficient);
		}
		return;
	}

	predictWholeBlock<16>(row, column, above, left, target,
			info->lumaMode);
	short residual[16], dc[16];
	if (info->lastCoeff[24] > 0) {
		inverseWalshHadamardTransform(coefficient[24], dc);
	} else {
		std::memset(dc, 0, sizeof(dc));
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (dc[i * 4 + j] == 0 && info->lastCoeff[i * 4 + j] == 0) {
				continue;
			}
			coefficient[i * 4 + j][0] = dc[i * 4 + j];
			inverseDiscreteCosineTransform(coefficient[i * 4 + j],
					residual);
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					Pixel* p = target + (i * 4 + y) * geometry->lumaWidth
							+ j * 4 + x;
					*p = clamp(*p + residual[y * 4 + x]);
				}
			}
		}
	}
}

void FrameBuffer::predictBusyLuma(Pixel* above, Pixel* left,
		Pixel* extra, Pixel* target, const BlockInfo* info,
		short coefficient[25][16]) {
	for (int row = 0; row < 4; ++row, above += 4 * geometry->lumaWidth) {
		Pixel subblock[4][4];
		short residual[16];
		left = above + geometry->lumaWidth - 1;
		for (int col = 0; col < 3; ++col, left += 4) {
			predict4x4(subblock, above + col * 4, left,
					above + col * 4 + 4, info->submode[row * 4 + col]);
			inverseDiscreteCosineTransform(coefficient[row * 4 + col],
					residual);
			addSubblockResidual(row, col, target, residual, subblock);
		}
		predict4x4(subblock, above + 12, left, extra,
				info->submode[row * 4 + 3]);
		inverseDiscreteCosineTransform(coefficient[4 * row + 3], residual);
		addSubblockResidual(row, 3, target, residual, subblock);
	}
}

void FrameBuffer::predict4x4(Pixel b[4][4], Pixel* above, Pixel* left,
		Pixel* extra, int8_t submode) {
	Pixel E[9];
	int lineSize = geometry->lumaWidth;
	E[0] = left[3 * lineSize]; E[1] = left[2 * lineSize];
	E[2] = left[lineSize]; E[3] = left[0];
	E[4] = above[-1]; E[5] = above[0]; E[6] = above[1]; E[7] = above[2];
	E[8] = above[3];
	switch (submode) {
	case kAverageSubmode: {
		short sum = 0;
		for (int i = 0; i < 4; ++i) {
			sum += above[i];
		}
		for (int i = 0; i < 4; ++i) {
			sum += left[i * lineSize];
		}
		std::memset(b, (sum + 4) / 8, 16);
		break; }
	case kHorizontalSubmode: {
		Pixel pixel = avg3(left[-lineSize], left[0], left[lineSize]);
		for (int j = 0; j < 4; ++j) {
			b[0][j] = pixel;
		}
		for (int i = 1; i < 3; ++i) {
			pixel = avg3(left[(i - 1) * lineSize], left[i * lineSize],
					left[(i + 1) * lineSize]);
			for (int j = 0; j < 4; ++j) {
				b[i][j] = pixel;
			}
		}
		pixel = avg3(left[2 * lineSize], left[3 * lineSize],
				left[3 * lineSize]);
		for (int j = 0; j < 4; ++j) {
			b[3][j] = pixel;
		}
		break; }
	case kVerticalSubmode:
		for (int i = 0; i < 4; ++i) {
			b[i][0] = avg3p(above);
			for (int j = 1; j < 3; ++j) {
				b[i][j] = avg3p(above + j);
			}
			b[i][3] = avg3(above[2], above[3], extra[0]);
		}
		break;
	case kTrueMotionSubmode:
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				b[i][j] = clamp(above[j] + left[i * lineSize] - above[-1]);
			}
		}
		break;
	case kLeftDownSubmode:
		b[0][0] = avg3(above[0], above[1], above[2]);
		b[0][1] = b[1][0] = avg3(above[1], above[2], above[3]);
		b[0][2] = b[1][1] = b[2][0] =
				avg3(above[2], above[3], extra[0]);
		b[0][3] = b[1][2] = b[2][1] = b[3][0] =
				avg3(above[3], extra[0], extra[1]);
		b[1][3] = b[2][2] = b[3][1] =
				avg3(extra[0], extra[1], extra[2]);
		b[2][3] = b[3][2] = avg3(extra[1], extra[2], extra[3]);
		b[3][3] = avg3(extra[2], extra[3], extra[3]);
		break;
	case kRightDownSubmode:
		b[3][0] = avg3p(E + 1);
		b[3][1] = b[2][0] = avg3p(E + 2);
		b[3][2] = b[2][1] = b[1][0] = avg3p(E + 3);
		b[3][3] = b[2][2] = b[1][1] = b[0][0] =
				avg3p(E + 4);
		b[2][3] = b[1][2] = b[0][1] = avg3p(E + 5);
		b[1][3] = b[0][2] = avg3p(E + 6);
		b[0][3] = avg3p(E + 7);
		break;
	case kHorizontalDownSubmode:
		b[3][0] = avg2p(E);
		b[3][1] = avg3p(E + 1);
		b[2][0] = b[3][2] = avg2p(E + 1);
		b[2][1] = b[3][3] = avg3p(E + 2);
		b[2][2] = b[1][0] = avg2p(E + 2);
		b[2][3] = b[1][1] = avg3p(E + 3);
		b[1][2] = b[0][0] = avg2p(E + 3);
		b[1][3] = b[0][1] = avg3p(E + 4);
		b[0][2] = avg3p(E + 5);
		b[0][3] = avg3p(E + 6);
		break;
	case kHorizontalUpSubmode:
		b[0][0] = avg2(left[0], left[lineSize]);
		b[0][1] = avg3(left[0], left[lineSize], left[lineSize * 2]);
		b[0][2] = b[1][0] = avg2(left[lineSize], left[lineSize * 2]);
		b[0][3] = b[1][1] = avg3(left[lineSize], left[lineSize * 2],
				left[lineSize * 3]);
		b[1][2] = b[2][0] = avg2(left[lineSize * 2], left[lineSize * 3]);
		b[1][3] = b[2][1] = avg3(left[lineSize * 2], left[lineSize * 3],
				left[lineSize * 3]);
		b[2][2] = b[2][3] = b[3][0] = b[3][1] = b[3][2] = b[3][3] =
				left[lineSize * 3];
		break;
	case kVerticalLeftSubmode:
		b[0][0] = avg2p(above);
		b[1][0] = avg3p(above + 1);
		b[2][0] = b[0][1] = avg2p(above + 1);
		b[1][1] = b[3][0] = avg3p(above + 2);
		b[2][1] = b[0][2] = avg2p(above + 2);
		b[3][1] = b[1][2] = avg3(above[2], above[3], extra[0]);
		b[2][2] = b[0][3] = avg2(above[3], extra[0]);
		b[3][2] = b[1][3] = avg3(above[3], extra[0], extra[1]);
		b[2][3] = avg3p(extra + 1);
		b[3][3] = avg3p(extra + 2);
		break;
	case kVerticalRightSubmode:
		b[3][0] = avg3p(E + 2);
		b[2][0] = avg3p(E + 3);
		b[3][1] = b[1][0] = avg3p(E + 4);
		b[2][1] = b[0][0] = avg2p(E + 4);
		b[3][2] = b[1][1] = avg3p(E + 5);
		b[2][2] = b[0][1] = avg2p(E + 5);
		b[3][3] = b[1][2] = avg3p(E + 6);
		b[2][3] = b[0][2] = avg2p(E + 6);
		b[1][3] = avg3p(E + 7);
		b[0][3] = avg2p(E + 7);
		break;
	default:
		RAISE(Unpossible);
	}
}

void FrameBuffer::predictChromaBlock(int row, int column,
		const BlockInfo* info, short coefficient[25][16]) {
	for (int p = 0; p < 2; ++p) {
		Pixel* above = chroma[p] + column * 8
				+ (row * 8 - 1) * geometry->chromaWidth;
		Pixel* target = above + geometry->chromaWidth;
		Pixel* left = target - 1;
		predictWholeBlock<8>(row, column, above, left, target,
				info->chromaMode);
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 2; ++j) {
				short residual[16];
				int id = 16 + p * 4 + i * 2 + j;
				inverseDiscreteCosineTransform(coefficient[id], residual);
				for (int y = 0; y < 4; ++y) {
					for (int x = 0; x < 4; ++x) {
						int offset = (i * 4 + y) * geometry->chromaWidth
								+ j * 4 + x;
						target[offset] = clamp(
								target[offset] + residual[y * 4 + x]);
					}
				}
			}
		}
	}
}

template<int BLOCK_SIZE>
void FrameBuffer::predictWholeBlock(int row, int column, Pixel* above,
		Pixel* left, Pixel* target, int8_t mode) {
	int lineSize = geometry->blockWidth * BLOCK_SIZE;
	switch (mode) {
	case kAverageMode: {
		uint32_t sum = 0;
		int total = 0;
		if (row > 1) {
			for (int i = 0; i < BLOCK_SIZE; ++i) {
				sum += above[i];
			}
			sum += BLOCK_SIZE / 2;
			total += BLOCK_SIZE;
		}
		if (column > 1) {
			for (int i = 0; i < BLOCK_SIZE; ++i) {
				sum += left[i * lineSize];
			}
			sum += BLOCK_SIZE / 2;
			total += BLOCK_SIZE;
		}
		Pixel average = (total == 0) ? 128 : sum / total;
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			std::memset(target + i * lineSize, average, BLOCK_SIZE);
		}
		break; }
	case kHorizontalMode:
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			std::memset(target + lineSize * i, target[lineSize * i - 1],
					BLOCK_SIZE);
		}
		break;
	case kVerticalMode:
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			std::memcpy(target + lineSize * i, above, BLOCK_SIZE);
		}
		break;
	case kTrueMotionMode:
		for (int i = 0; i < BLOCK_SIZE; ++i) {
			for (int j = 0; j < BLOCK_SIZE; ++j) {
				target[i * lineSize + j] =
						clamp(above[j] + left[i * lineSize] - above[-1]);
			}
		}
		break;
	default:
		RAISE(Unpossible);
	}
}

void FrameBuffer::addSubblockResidual(int row, int column, Pixel* target,
		const short* residual, Pixel subblock[4][4]) {
	for (int i = 0; i < 4; ++i) {
		int offset = (row * 4 + i) * geometry->lumaWidth + column * 4;
		for (int j = 0; j < 4; ++j) {
			target[offset + j] =
					clamp(subblock[i][j] + residual[i * 4 + j]);
		}
	}
}

void FrameBuffer::writeDisplay(Display* display) {
	for (int i = 0; i < geometry->displayHeight; ++i) {
		display->write(0,
				luma + (i + 16) * geometry->lumaWidth + 16,
				geometry->displayWidth);
	}
	for (int p = 0; p < 2; ++p) {
		for (int i = 0; i < geometry->displayHeight / 2; ++i) {
			Pixel* src = chroma[p] + 8
					+ (i + 8) * geometry->chromaWidth;
			display->write(p + 1, src, geometry->displayWidth / 2);
		}
	}
	display->flush();
}
