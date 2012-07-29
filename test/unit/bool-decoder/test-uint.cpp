#include <stdint.h>
#include <cstdio>
#include <cstddef>
#include <cherry/decode/BoolDecoder.hpp>
#include <gtest/gtest.h>

using namespace cherry;

// The initialization function of the "canonical" bool decoder is flawed,
// our implementation should have the same behaviour. In a "perfect"
// decoder, the calls to BoolDecoder::uint<8>() should return the sequence
// 0x80, 0xf3, 0xf2, 0xf1
TEST(BoolDecoder, flawedInitializer) {
	int data = 0xf1f2f380;
	const static int result[] = {0x80, 0xf5, 0xde, 0xae};
	BoolDecoder decoder;
	decoder.reload(&data, sizeof(data));
	for (size_t i = 0; i < sizeof(data); ++i) {
		ASSERT_EQ(decoder.uint<8>(), result[i]) << "Unexpected byte at "
				<< i;
	}
}

TEST(BoolDecoder, returnInputStream) {
	int data = 0xf1f2f37f;
	const static int result[] = {0x7f, 0xf3, 0xf2, 0xf1};
	BoolDecoder decoder;
	decoder.reload(&data, sizeof(data));
	for (size_t i = 0; i < sizeof(data); ++i) {
		ASSERT_EQ(decoder.uint<8>(), result[i]) << "Unexpected byte at"
				<< i;
	}
}
