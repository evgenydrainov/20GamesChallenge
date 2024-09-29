#pragma once

#include "common.h"
#include "texture.h"

constexpr size_t BATCH_MAX_QUADS    = 10'000;
constexpr size_t VERTICES_PER_QUAD  = 4;
constexpr size_t INDICES_PER_QUAD   = 6;
constexpr size_t BATCH_MAX_VERTICES = (BATCH_MAX_QUADS * VERTICES_PER_QUAD);
constexpr size_t BATCH_MAX_INDICES  = (BATCH_MAX_QUADS * INDICES_PER_QUAD);

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec4 color;
	vec2 uv;
};

enum RenderMode {
	MODE_NONE,
	MODE_QUADS,
	MODE_TRIANGLES,
};

struct Batch_Renderer {
	u32 current_texture;
	RenderMode current_mode;
	bump_array<Vertex> batch_vertices;

	u32 texture_shader;

	u32 batch_vao;
	u32 batch_vbo;
	u32 batch_ebo;
	u32 stub_texture;

	mat4 proj_mat = {1};
	mat4 view_mat = {1};
	mat4 model_mat = {1};

	int draw_calls;
	size_t max_batch;

	int curr_draw_calls;
	size_t curr_max_batch;
};

extern Batch_Renderer renderer;

void init_renderer();
void deinit_renderer();

void render_begin_frame(vec4 clear_color);
void render_end_frame();

void break_batch();

void draw_texture(Texture t, Rect src = {},
				  vec2 pos = {}, vec2 scale = {1, 1},
				  vec2 origin = {}, float angle = 0, vec4 color = color_white, glm::bvec2 flip = {});

void draw_rectangle(Rectf rect, vec4 color);

void draw_rectangle(Rectf rect, vec2 scale,
					vec2 origin, float angle, vec4 color);

void draw_triangle(vec2 p1, vec2 p2, vec2 p3, vec4 color);

void draw_circle(vec2 pos, float radius, vec4 color, int precision = 12);
