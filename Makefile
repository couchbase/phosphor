# -*- Mode: makefile -*-

CMAKE=cmake
MAKETYPE=NMake Makefiles
EXTRA_CMAKE_OPTIONS=
CMAKE_ARGS=$(EXTRA_CMAKE_OPTIONS)

all: test docs

covered: coverage test

coverage-report: covered
	@-mkdir coverage
	gcovr --root . --html --html-details -o coverage/index.html --exclude="thirdparty*" --exclude="tests*"
	open coverage/index.html

build/Makefile:
	@-mkdir build
	(cd build && $(CMAKE) $(CMAKE_ARGS) ..)

compile: build/Makefile
	(cd build && $(CMAKE) --build .)

.PHONY: coverage
coverage:
	find . -name *.gcda -exec rm {} \;
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DCB_CODE_COVERAGE=ON)

coveralls:
	coveralls

test: compile
	(cd build && ctest --output-on-failure)

docs:
	doxygen > /dev/null

clean:
	rm -rf build
