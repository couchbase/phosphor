# -*- Mode: makefile -*-

CMAKE=cmake
MAKETYPE=NMake Makefiles
EXTRA_CMAKE_OPTIONS=
CMAKE_ARGS=$(EXTRA_CMAKE_OPTIONS)

all: property-benchmark property-examples property-coverage test

covered: property-coverage test

thread-sanitizer: property-tsan test

address-sanitizer: property-asan test

benchmark: property-benchmark test

coverage-report: covered
	@-mkdir coverage
	gcovr --root . --html --html-details -o coverage/index.html --exclude="thirdparty*" --exclude="tests*"
	open coverage/index.html

build/Makefile:
	@-mkdir build
	(cd build && $(CMAKE) $(CMAKE_ARGS) ..)

compile: build/Makefile
	(cd build && $(CMAKE) --build .)

property-coverage:
	find . -name *.gcda -exec rm {} \;
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DCB_CODE_COVERAGE=ON)

property-tsan:
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DCB_THREADSANITIZER=ON)

property-asan:
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DCB_ADDRESSSANITIZER=ON)

property-benchmark:
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DPHOSPHOR_ENABLE_BENCHMARKING=1)

property-examples:
	$(eval EXTRA_CMAKE_OPTIONS:=$(EXTRA_CMAKE_OPTIONS) -DPHOSPHOR_BUILD_EXAMPLES=1)

coveralls:
	coveralls

test: compile
	(cd build && ctest --output-on-failure)

docs:
	doxygen > /dev/null

clean:
	rm -rf build

format:
	clang-format -style=file -i $$(git ls-files | grep -v thirdparty | grep -v dyn_array.h | grep -E "\.cc|\.h");
