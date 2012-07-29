REQUIRE = ../../source
include ../../build/include.mk

LIBS = $(PROJECT_ROOT)/external/gtest/make/gtest.a \
	$(BINARY_ROOT)/source/libcherry.a
TESTS = bool-decoder/test-uint.cpp

EXEC = $(patsubst %.o,%,$(call OBJECT_LIST, $(TESTS)))
MARKER = $(addsuffix .pass,$(EXEC))
MAIN = $(call OBJECT_LIST, main.cpp)

include $(call INCLUDE_LIST, $(TESTS) main.cpp)

all: $(EXEC) $(MARKER)

%: %.o $(LIBS) $(MAIN)
	$(CXX) -o $@ -Wl,--start-group $^ -Wl,--end-group -pthread

%.pass: %
	$* && touch $@
