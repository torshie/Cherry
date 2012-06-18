ifndef PROJECT_ROOT
    PROJECT_ROOT := $(shell \
        while [ ! -f build/include.mk ]; do \
            cd ..; \
        done; \
        pwd \
    )
    export PROJECT_ROOT
endif

all: build

build: build-required

include $(PROJECT_ROOT)/build/check-prereq.mk
ifeq ($(MAKECMDGOALS),check-prereq)
    -include $(PROJECT_ROOT)/build/expensive-check.mk
endif

include $(PROJECT_ROOT)/build/config.mk
include $(PROJECT_ROOT)/build/common.mk
include $(PROJECT_ROOT)/build/target.mk
