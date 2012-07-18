include ../build/include.mk

SOURCE = decode/BoolDecoder.cpp \
	except/BasicExcept.cpp \
	unpack/IvfUnpacker.cpp \
	vp8/DecoderDriver.cpp \
	vp8/FrameBuffer.cpp \
	vp8/FilterParameter.cpp \
	display/DummyDisplay.cpp \
	dsp/dsp.cpp

-include $(call INCLUDE_PATH, $(SOURCE))

LIBS = $(BINARY_DIRECTORY)/libcherry.a

build: build-required $(LIBS)

$(LIBS): $(call OBJECT_PATH, $(SOURCE))
	$(AR) rcs $@ $^
