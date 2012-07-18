CXXFLAGS += -W -Wall -Wextra \
	$(INCLUDES) $(DEFINES) $(EXTRA_FLAGS) 

ifneq ($(BUILD_MODE),debug)
    CXXFLAGS += -O2 -g
else
    CXXFLAGS += -O0 -g
endif

ifneq ($(BUILD_MODE),debug)
    ifneq ($(or $(findstring g++,$(CXX)), $(findstring gcc,$(CXX))),)
        CXXFLAGS += -flto
    endif
    ifneq ($(findstring clang,$(CXX)),)
        CXXFLAGS += -flto
    endif
endif

RELATIVE_PATH = $(subst $(PROJECT_ROOT),,$(CURDIR))

BINARY_DIRECTORY = $(BINARY_ROOT)$(RELATIVE_PATH)

OBJECT_PATH = $(patsubst %.cpp,%.o,$(addprefix $(BINARY_DIRECTORY)/,$(1)))

INCLUDE_PATH = $(patsubst %.o,%.dep,$(call OBJECT_PATH,$(1)))
