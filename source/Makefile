include ../build/include.mk

GENERATED = generator/BoolEncoder_functionTable.cpp

SOURCE = decode/BoolDecoder.cpp \
	encode/BoolEncoder.cpp \
	except/BasicExcept.cpp \
	unpack/IvfUnpacker.cpp \
	vp8/DecoderDriver.cpp \
	vp8/FrameBuffer.cpp \
	dsp/dsp.cpp \
	$(GENERATED)

-include $(call INCLUDE_PATH, $(SOURCE))

LIBS = $(BINARY_DIRECTORY)/libcherry.a

build: build-required $(LIBS)

$(LIBS): $(call OBJECT_PATH, $(SOURCE))
	$(AR) rcs $@ $^
