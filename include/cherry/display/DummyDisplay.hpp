#ifndef _CHERRY_DISPLAY_DUMMY_DISPLAY_HPP_INCLUDED_
#define _CHERRY_DISPLAY_DUMMY_DISPLAY_HPP_INCLUDED_

#include <cstdio>
#include <cherry/vp8/types.hpp>
#include <cherry/display/Display.hpp>

namespace cherry {

class DummyDisplay : public Display {
public:
	explicit DummyDisplay(const char* filename);
	virtual ~DummyDisplay();

	virtual void resize(int width, int height);
	virtual size_t write(int id, const void* data, size_t length);
	virtual void flush();

private:
	int width, height;
	FILE* output;
	Pixel* buffer[3];
	size_t size[3];
	size_t offset[3];
};

} // namespace cherry

#endif // _CHERRY_DISPLAY_DUMMY_DISPLAY_HPP_INCLUDED_
