#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdio.h>


inline void print_vec(const char* name, Vec2 v) {
    printf("%s: %.2f, %.2f\n", name, v.x, v.y);
}

inline void vec3_print(const char* name, Vec3 v) {
    printf("%s: %.2f, %.2f, %.2f\n", name, v.x, v.y, v.z);
}

inline void print_affine(const char* name, Affine m) {
    printf("%s:\n", name);
    printf("%.2f %.2f %.2f\n", m.m.m00, m.m.m01, m.t.x);
    printf("%.2f %.2f %.2f\n", m.m.m10, m.m.m11, m.t.y);
}

inline void print_rgba(const char* name, RGBA c) {
    printf("%s: %.2f, %.2f, %.2f, %.2f\n", name, c.r, c.g, c.b, c.a);
}

inline float randf() {
    return (float)rand() / (float)RAND_MAX;
}

#endif /* UTIL_HPP */
