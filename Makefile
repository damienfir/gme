build/main: main_glfw.cpp
	mkdir build || true
	g++ -g -lglfw -lGL -lGLU -lGLEW -std=c++17 -Wall $^ -o $@

clean:
	rm -r build/main
