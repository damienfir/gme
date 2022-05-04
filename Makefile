build/main: main_X11.cpp
	-mkdir build
	g++ -g -lX11 -lGL -lGLU -lGLEW -std=c++17 -Wall $^ -o $@

clean:
	rm -r build/main
