all: compile docs

compile:
	(cd build && cmake .. && make -j8)

docs:
	doxygen
