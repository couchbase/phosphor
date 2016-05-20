# -*- Mode: makefile -*-

CMAKE=cmake
MAKETYPE=NMake Makefiles
EXTRA_CMAKE_OPTIONS=
CMAKE_ARGS=$(EXTRA_CMAKE_OPTIONS)

all: test docs

covered: coverage test

build/Makefile:
	@-mkdir build
	(cd build && $(CMAKE) $(CMAKE_ARGS) ..)

compile: build/Makefile
	(cd build && make)

coverage:
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DCB_CODE_COVERAGE=ON)

coverage-html:
	(cd build && make build-coverage-report-html)

coveralls:
	coveralls

test: compile
	(cd build && make test -j8)

docs:
	doxygen > /dev/null

clean:
	rm -rf build
