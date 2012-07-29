REQUIRE = source tools
include build/include.mk

check-prereq:
	@echo "Checking done. Good luck."

check:
	$(MAKE) -C test
