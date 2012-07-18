#ifndef _CHERRY_DISPLAY_DISPLAY_HPP_INCLUDED_
#define _CHERRY_DISPLAY_DISPLAY_HPP_INCLUDED_

namespace cherry {

class Display {
public:
	virtual ~Display() {}

	virtual void resize(int width, int height) = 0;
	virtual size_t write(int id, const void* data, size_t length) = 0;
	virtual void flush() = 0;
};

} // namespace cherry

#endif // _CHERRY_DISPLAY_DISPLAY_HPP_INCLUDED_
