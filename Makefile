all: build/portal2d

%.o: %.cpp %.hpp
	clang -g -std=c++17 -Wall -Og -c $< -o $@

build/main: main.o gfx.o world.o math.o
	mkdir build || true
	clang -g -lm -lglfw -lGL -lGLU -lGLEW -lpng -lstdc++ $^ -o $@

build/portal2d: portal2d.o img.o math.o gfx.o
	mkdir build || true
	clang -g -lm -lglfw -lGL -lGLU -lGLEW -lpng -lstdc++ $^ -o $@

build/random_walk: random_walk.o gfx.o math.o
	mkdir build || true
	clang -g -lm -lglfw -lGL -lGLU -lGLEW -lpng -lstdc++ $^ -o $@

build/slime: slime.o gfx.o math.o img.o
	mkdir build || true
	clang -g -lm -lglfw -lGL -lGLU -lGLEW -lpng -lstdc++ $^ -o $@

clean:
	-rm *.o
	-rm -r build/main
	-rm -r build/portal2d
	-rm -r build/random_walk
	-rm -r build/slime
