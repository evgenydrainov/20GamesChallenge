#pragma once

// Cirno's Perfect Math Library

#include <math.h>

#define PI 3.1415926535897932384626433832795f

struct Vector2 { float x, y; };

template <typename T>
static T min(T a, T b) {
	return (a < b) ? a : b;
}

template <typename T>
static T max(T a, T b) {
	return (a > b) ? a : b;
}

template <typename T>
static T clamp(T x, T _min, T _max) {
	if (x > _max) {
		x = _max;
	}

	if (x < _min) {
		x = _min;
	}

	return x;
}

template <typename T>
static T approach(T start, T end, T shift) {
	if (end > start) {
		start += shift;
		if (start > end) start = end;
	} else if (end < start) {
		start -= shift;
		if (start < end) start = end;
	}
	return start;
}

static float lerp(float a, float b, float f) {
	return a + (b - a) * f;
}

static int wrap(int a, int b) {
	return (a % b + b) % b;
}

static float length(float x, float y) {
	return sqrtf(x * x + y * y);
}

static float to_radians(float deg) {
	return deg / 180.0f * PI;
}

static float to_degrees(float rad) {
	return rad / PI * 180.0f;
}

static float dcos(float deg) {
	return cosf(to_radians(deg));
}

static float dsin(float deg) {
	return sinf(to_radians(deg));
}

static float lengthdir_x(float len, float dir) {
	return dcos(dir) * len;
}

static float lengthdir_y(float len, float dir) {
	return -dsin(dir) * len;
}

static float angle_wrap(float deg) {
	deg = fmodf(deg, 360.0f);
	if (deg < 0.0f) {
		deg += 360.0f;
	}
	return deg;
}

static float angle_difference(float dest, float src) {
	float res = dest - src;
	res = angle_wrap(res + 180.0f) - 180.0f;
	return res;
}

static Vector2 normalize0(Vector2 v) {
	float l = length(v.x, v.y);
	if (l == 0.0f) {
		return {};
	} else {
		return {v.x / l, v.y / l};
	}
}

static float point_direction_rad(float x1, float y1, float x2, float y2) {
	return atan2f(y1 - y2, x2 - x1);
}

static float point_direction(float x1, float y1, float x2, float y2) {
	return to_degrees(point_direction_rad(x1, y1, x2, y2));
}

static float point_distance(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	return length(dx, dy);
}

static bool circle_vs_circle(float x1, float y1, float r1, float x2, float y2, float r2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float r = r1 + r2;
	return (dx * dx + dy * dy) < (r * r);
}
