include ../build/include.mk

SOURCE = block-scan.cpp
-include $(call INCLUDE_PATH, $(SOURCE))

EXE = $(patsubst %.cpp,%,$(SOURCE))

build: $(addprefix $(BINARY_DIRECTORY)/,$(EXE))

%: %.o
	$(CXX) -o $@ -Wl,--start-group $^ -Wl,--end-group $(CXXFLAGS)
