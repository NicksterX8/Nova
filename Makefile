build_it:
	(cd build && make -j && ctest --output-on-failure)