%.o: %.cpp %.hpp
	clang -g -std=c++17 -Wall -Og -c $< -o $@

build/main: main.o gfx.o world.o math.o
	mkdir build || true
	clang -lm -lglfw -lGL -lGLU -lGLEW -lpng -lstdc++ $^ -o $@

clean:
	-rm *.o
	-rm -r build/main
