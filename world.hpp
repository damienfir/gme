#ifndef WORLD_HPP
#define WORLD_HPP

void world_init();
void world_key_input(int action, int key);
void world_mouse_cursor_position(float xpos, float ypos);
void world_mouse_button(int button, int action);
void world_scroll_input(float xoffset, float yoffset);
void world_update(float dt);
void world_draw();

#endif /* WORLD_HPP */
