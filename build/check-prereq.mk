ifeq ($(PROJECT_ROOT),)
    $(error PROJECT_ROOT is undefined, which is very weird.)
endif
ifneq ($(words $(PROJECT_ROOT)),1)
    $(error Absolute path of the project root directory contains space(s))
endif
