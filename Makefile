build/main: main_glfw.cpp
	mkdir build || true
	g++ -g -lglfw -lGL -lGLU -lGLEW -lpng -std=c++17 -Wall -O2 $^ -o $@

clean:
	rm -r build/main
