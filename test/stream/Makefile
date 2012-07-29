REQUIRE = ../../source
include ../../build/include.mk

DECODER = $(abspath $(BINARY_DIRECTORY)/../../source/cherry-decoder)
IVF_LIST = one-frame/simple-filter.ivf \
	one-frame/normal-filter.ivf
FULL_PATH = $(addprefix $(BINARY_DIRECTORY)/,$(IVF_LIST))

all: build-required $(patsubst %.ivf,%.y4m,$(FULL_PATH)) \
	$(patsubst %.ivf,%.pass,$(FULL_PATH))

%.pass: %.y4m
	$(PROJECT_ROOT)/tools/validate-checksum.sh $(CURDIR)/checksum.md5 $< \
		&& touch $@

$(BINARY_DIRECTORY)/%.y4m: %.ivf $(DECODER)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(DECODER) $< $(@)
