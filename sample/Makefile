REQUIRE = ../source
include ../build/include.mk

SOURCE = unpack-ivf.cpp decode-first.cpp bool-decoder.cpp
-include $(call INCLUDE_PATH, $(SOURCE))

EXE = $(patsubst %.cpp,%,$(SOURCE))

build: build-required $(addprefix $(BINARY_DIRECTORY)/,$(EXE))

%: %.o $(BINARY_ROOT)/source/libcherry.a
	$(CXX) -o $@ -Wl,--start-group $^ -Wl,--end-group $(CXXFLAGS)
