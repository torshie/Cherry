include ../build/include.mk

SOURCE = decode/BoolDecoder.cpp \
	except/BasicExcept.cpp \
	unpack/IvfUnpacker.cpp \
	vp8/MainDecoder.cpp \
	vp8/FrameBuffer.cpp \
	vp8/LoopFilter.cpp \
	vp8/ProbabilityManager.cpp \
	display/DummyDisplay.cpp \
	dsp/dsp.cpp
DECODER = cherry-decoder.cpp

-include $(call INCLUDE_LIST, $(SOURCE) $(DECODER))

LIBS = $(BINARY_DIRECTORY)/libcherry.a
EXEC = $(BINARY_DIRECTORY)/$(patsubst %.cpp,%,$(DECODER))

build: build-required $(LIBS) $(EXEC)

$(LIBS): $(call OBJECT_LIST, $(SOURCE))
	$(AR) rcs $@ $^

$(EXEC): $(call OBJECT_LIST, $(DECODER)) $(LIBS)
	$(CXX) -o $@ $^

