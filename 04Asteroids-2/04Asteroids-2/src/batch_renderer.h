#pragma once

#include "common.h"

constexpr size_t BATCH_MAX_QUADS    = 10'000;
constexpr size_t VERTICES_PER_QUAD  = 4;
constexpr size_t INDICES_PER_QUAD   = 6;
constexpr size_t BATCH_MAX_VERTICES = (BATCH_MAX_QUADS * VERTICES_PER_QUAD);
constexpr size_t BATCH_MAX_INDICES  = (BATCH_MAX_QUADS * INDICES_PER_QUAD);

constexpr vec4 color_white  = { 1.00f, 1.00f, 1.00f, 1.00f };
constexpr vec4 color_black  = { 0.00f, 0.00f, 0.00f, 1.00f };
constexpr vec4 color_red    = { 1.00f, 0.00f, 0.00f, 1.00f };
constexpr vec4 color_green  = { 0.00f, 1.00f, 0.00f, 1.00f };
constexpr vec4 color_blue   = { 0.00f, 0.00f, 1.00f, 1.00f };
constexpr vec4 color_yellow = { 1.00f, 1.00f, 0.00f, 1.00f };

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec4 color;
	vec2 uv;
};

struct Texture {
	u32 ID;
	int width;
	int height;
};

extern mat4 proj_mat;
extern mat4 view_mat;
extern mat4 model_mat;

void init_renderer();
void deinit_renderer();

void break_batch();

void draw_texture(Texture t, Rect src = {},
				  vec2 pos = {}, vec2 scale = {1, 1},
				  vec2 origin = {}, float angle = 0, vec4 color = color_white, glm::bvec2 flip = {});

void draw_rectangle(Rectf rect, vec4 color = color_white);

void draw_rectangle(Rectf rect, vec2 scale,
					vec2 origin = {}, float angle = 0, vec4 color = color_white);

void draw_triangle(vec2 p1, vec2 p2, vec2 p3, vec4 color = color_white);

void draw_circle(vec2 pos, float radius, vec4 color = color_white, int precision = 12);
