build/main: main_glfw.cpp math.hpp image.hpp
	mkdir build || true
	g++ -g -lglfw -lGL -lGLU -lGLEW -lpng -std=c++17 -Wall -O2 $< -o $@

build/tests: tests.cpp math.hpp
	mkdir build || true
	g++ -g -std=c++17 -Wall -O2 $< -o $@

clean:
	rm -r build/main
