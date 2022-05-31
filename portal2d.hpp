#ifndef PORTAL2D_HPP
#define PORTAL2D_HPP

void portal2d_init();
void portal2d_key_input(int action, int key);
void portal2d_mouse_cursor_position(float xpos, float ypos);
void portal2d_mouse_button(int button, int action);
void portal2d_update(float dt);
void portal2d_draw();

#endif /* PORTAL2D_HPP */
