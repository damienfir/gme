project(gme)

add_executable(portal2d
    portal2d.cpp
    gfx.cpp
    img.cpp
    math.cpp
    )
target_link_libraries(portal2d glfw GL GLU GLEW png)

add_executable(world2
    world2.cpp
    gfx.cpp
    img.cpp
    math.cpp
    perlin.cpp
    )
target_link_libraries(world2 glfw GL GLU GLEW png)
