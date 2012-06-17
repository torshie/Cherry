#ifndef _CHERRY_DECODE_BOOLEAN_DECODER_HPP_INCLUDED_
#define _CHERRY_DECODE_BOOLEAN_DECODER_HPP_INCLUDED_

namespace cherry {

struct bool_decoder {
	const unsigned char* input; /* next compressed data byte */
	size_t input_len; /* length of the input buffer */
	unsigned int range; /* identical to encoder’s range */
	unsigned int value; /* contains at least 8 significant bits */
	int bit_count; /* # of bits shifted out of value, max 7 */
};

} // namespace cherry

#endif // _CHERRY_DECODE_BOOLEAN_DECODER_HPP_INCLUDED_
