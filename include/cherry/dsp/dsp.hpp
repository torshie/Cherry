#ifndef _CHERRY_DSP_DSP_HPP_INCLUDED_
#define _CHERRY_DSP_DSP_HPP_INCLUDED_

namespace cherry {

void inverseWalshHadamardTransform(const short* input, short* output);
void inverseDiscreteCosineTransform(const short* input, short* output);

} // namespace cherry

#endif // _CHERRY_DSP_DSP_HPP_INCLUDED_
