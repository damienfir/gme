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

struct RGBA {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;
};

RGBA operator*(float a, RGBA v) {
    return RGBA{v.r*a, v.g*a, v.b*a, v.a*a};
}

RGBA operator+(RGBA a, RGBA b) {
    return RGBA{a.r+b.r, a.g+b.g, a.b+b.b, a.a+b.a};
}

RGBA& operator+=(RGBA &a, RGBA b) {
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    a.a += b.a;
    return a;
}

RGBA clamp01(RGBA a) {
    RGBA b;
    b.r = std::min(1.f, std::max(0.f, a.r));
    b.g = std::min(1.f, std::max(0.f, a.g));
    b.b = std::min(1.f, std::max(0.f, a.b));
    b.a = std::min(1.f, std::max(0.f, a.a));
    return b;
}
