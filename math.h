#pragma once

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

float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}
