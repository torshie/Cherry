include ../build/include.mk

SOURCE = block-scan.cpp
-include $(call INCLUDE_LIST, $(SOURCE))

EXE = $(patsubst %.cpp,%,$(SOURCE))

build: $(addprefix $(BINARY_DIRECTORY)/,$(EXE))

%: %.o
	$(CXX) -o $@ $^ $(CXXFLAGS)
