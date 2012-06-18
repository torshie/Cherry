REQUIRE = ../source
include ../build/include.mk

SOURCE = bool-codec.cpp
-include $(call INCLUDE_PATH, $(SOURCE))

EXE = bool-codec.exe

build: build-required $(addprefix $(BINARY_DIRECTORY)/,$(EXE))

%.exe: %.o $(BINARY_ROOT)/source/libcherry.a
	$(CXX) -o $@ -Wl,--start-group $^ -Wl,--end-group $(CXXFLAGS)
