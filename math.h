#pragma once

#include <cmath>

struct Vec2 {
    float x;
    float y;
};

Vec2 operator-(Vec2 a) {
    return Vec2{-a.x, -a.y};
}

Vec2 operator-(Vec2 a, Vec2 b) {
    return Vec2{a.x-b.x, a.y-b.y};
}

Vec2 operator+(Vec2 a, Vec2 b) {
    return Vec2{a.x+b.x, a.y+b.y};
}

float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

Vec2 operator/(Vec2 v, float d) {
    return Vec2{v.x/d, v.y/d};
}

Vec2 operator*(Vec2 v, float d) {
    return Vec2{v.x*d, v.y*d};
}

Vec2 operator*(float d, Vec2 v) {
    return v * d;
}

float norm(Vec2 v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

Vec2 normalize(Vec2 v) {
    return v / norm(v);
}

Vec2 rotate(Vec2 v, float rad) {
    Vec2 w;
    w.x = v.x * cos(rad) + v.y * sin(rad);
    w.y = v.y * cos(rad) - v.x * sin(rad);
    return w;
}

struct Vec3 {
    float x;
    float y;
    float z;
};
