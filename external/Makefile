SVN_REPO = gtest

gtest_LOCATION = http://googletest.googlecode.com/svn/tags/release-1.6.0
gtest_REVISION = 619

all: checkout build

checkout: $(addsuffix _svn-checkout-phony,$(SVN_REPO))

build: $(addsuffix _build-phony,$(SVN_REPO))

%_svn-checkout-phony:
	if [ ! -d $* ]; then \
		svn checkout $($(*)_LOCATION) $*; \
	fi
	svn update -r $($(*)_REVISION)

gtest_build-phony:
	make -C gtest/make gtest.a
