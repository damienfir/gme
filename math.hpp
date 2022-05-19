#ifndef MATH_HPP
#define MATH_HPP

#pragma once

#include <cmath>
#include <cassert>

struct Mat2 {
    float m00 = 1;
    float m01 = 0;
    float m10 = 0;
    float m11 = 1;
};

struct Vec2 {
    float x = 0;
    float y = 0;
};

Vec2 operator-(Vec2 a) {
    return Vec2{-a.x, -a.y};
}

Vec2 operator-(Vec2 a, Vec2 b) {
    return Vec2{a.x-b.x, a.y-b.y};
}

Vec2 operator-(Vec2 a, float b) {
    return Vec2{a.x-b, a.y-b};
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

int modulo_positive(int x, int m) {
    int y = x % m;
    if (y < 0) return y + m;
    return y;
}

int clamp(int x, int mi, int ma) {
    return std::min(ma, std::max(mi, x));
}

float clamp(float x, float mi, float ma) {
    return std::min(ma, std::max(mi, x));
}


struct DistanceResult {
    float distance;
    bool within_segment;
};

DistanceResult distance_point_line(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 AB = normalize(b - a);
    Vec2 AP = p - a;
    Vec2 BP = p - b;

    Vec2 perp = Vec2{AB.y, -AB.x};
    DistanceResult res;
    res.within_segment = false;
    res.distance = dot(AP, perp);

    if (dot(AB, AP) < 0) {
        return res;
    }

    if (dot(-AB, BP) < 0) {
        return res;
    }

    res.within_segment = true;
    return res;
}

bool crossed_line(Vec2 pos_prev, Vec2 pos_next, Vec2 line_a, Vec2 line_b) {
    Vec2 BA = normalize(line_b - line_a);
    Vec2 PA = normalize(pos_next - line_a);
    Vec2 PB = normalize(pos_next - line_b);

    if (dot(BA, PA) < 0 || dot(-BA, PB) < 0) {
        return false;
    }

    Vec2 perp = Vec2{BA.y, -BA.x};
    float dist_prev = dot((pos_prev - line_a), perp);
    float dist_next = dot(PA, perp);
    if ((dist_prev > 0 && dist_next <= 0) || 
            (dist_prev < 0 && dist_next >= 0)) {
        return true;
    }

    return false;
}

bool crossed_line2(Vec2 pos_prev, Vec2 pos_next, float radius, Vec2 a, Vec2 b) {
    DistanceResult d_prev = distance_point_line(pos_prev, a, b);
    DistanceResult d_next = distance_point_line(pos_next, a, b);

    if (!d_prev.within_segment and !d_next.within_segment) {
        return false;
    }

    float d0 = d_prev.distance;
    float d1 = d_next.distance;
    if (d0 > 0 and d1 <= 0) {
        return true;
    }
    if (d0 <= 0 and d1 > 0) {
        return true;
    }
    if (std::abs(d0) >= radius and std::abs(d1) < radius) {
        return true;
    }

    return false;
}

struct Affine {
    Mat2 m;
    Vec2 t;
};

Affine from_translation(Vec2 v) {
    return {.t = v};
}

Affine from_rotation(float rad) {
    Affine a;
    a.m.m00 = std::cos(rad);
    a.m.m01 = -std::sin(rad);
    a.m.m10 = std::sin(rad);
    a.m.m11 = std::cos(rad);
    return a;
}

Affine from_scale(float s) {
    Affine a;
    a.m.m00 = s;
    a.m.m11 = s;
    return a;
}

Vec2 mul(Mat2 a, Vec2 v) {
    Vec2 w;
    w.x = a.m00 * v.x + a.m01 * v.y;
    w.y = a.m10 * v.x + a.m11 * v.y;
    return w;
}

Vec2 mul(Affine a, Vec2 v) {
    return mul(a.m, v) + a.t;
}

void test_mul_vec() {
    Affine t = from_scale(2);
    Vec2 a = {1, 2};
    Vec2 b = mul(t, a);
    assert(b.x == 2);
    assert(b.y == 4);
}

Mat2 mul(Mat2 a, Mat2 b) {
    Mat2 c;
    c.m00 = a.m00*b.m00 + a.m01*b.m10;
    c.m01 = a.m00*b.m01 + a.m01*b.m11;
    c.m10 = a.m10*b.m00 + a.m11*b.m10;
    c.m11 = a.m10*b.m01 + a.m11*b.m11;
    return c;
}

Affine mul(Affine a, Affine b) {
    // Derived it myself, it's probably wrong
    Affine c;
    c.m = mul(a.m, b.m);
    c.t = mul(a.m, b.t) + a.t;
    return c;
}

void test_mul() {
    Affine a = from_translation(Vec2{1, 3});
    Affine b = from_scale(2);
    Affine c = mul(a, b);
    assert(c.m.m00 == 2);
    assert(c.m.m01 == 0);
    assert(c.m.m10 == 0);
    assert(c.m.m11 == 2);
    assert(c.t.x == 1);
    assert(c.t.y == 3);
}

Mat2 inverse(Mat2 a) {
    Mat2 b;
    float det = a.m00*a.m11 - a.m10*a.m01;
    b.m00 = a.m11 / det;
    b.m01 = -a.m01 / det;
    b.m10 = -a.m10 / det;
    b.m11 = a.m00 / det;
    return b;
}

Affine inverse(Affine a) {
    Affine b;
    b.m = inverse(a.m);
    b.t = -mul(b.m, a.t);
    return b;
}

#endif /* MATH_HPP */
