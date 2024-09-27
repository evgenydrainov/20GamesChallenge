#include "batch_renderer.h"

#include "package.h"

enum RenderMode {
	MODE_NONE,
	MODE_QUADS,
	MODE_TRIANGLES,
};

static u32 current_texture = 0;
static RenderMode current_mode = MODE_NONE;
static bump_array<Vertex> batch_vertices = {};

static u32 texture_shader;
// static u32 color_shader;

static u32 batch_vao;
static u32 batch_vbo;
static u32 batch_ebo;
static u32 stub_texture;

mat4 proj_mat = {1};
mat4 view_mat = {1};
mat4 model_mat = {1};

static int draw_calls = 0;
static size_t max_batch = 0;


static char texture_vert_shader_src[] = R"(
	#version 330 core

	layout(location = 0) in vec3 in_Position;
	layout(location = 1) in vec3 in_Normal;
	layout(location = 2) in vec4 in_Color;
	layout(location = 3) in vec2 in_TexCoord;

	out vec4 v_Color;
	out vec2 v_TexCoord;

	uniform mat4 u_MVP;

	void main() {
		gl_Position = u_MVP * vec4(in_Position, 1.0);

		v_Color    = in_Color;
		v_TexCoord = in_TexCoord;
	}
)";



static char texture_frag_shader_src[] = R"(
	#version 330 core

	layout(location = 0) out vec4 FragColor;

	in vec4 v_Color;
	in vec2 v_TexCoord;

	uniform sampler2D u_Texture;

	void main() {
		vec4 color = texture(u_Texture, v_TexCoord);

		FragColor = color * v_Color;
	}
)";


static void set_vertex_attribs() {
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	glEnableVertexAttribArray(0);

	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);

	// Color
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(2);

	// Texcoord
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(3);
}

void init_renderer() {
	// 
	// Initialize.
	// 
	{
		glGenVertexArrays(1, &batch_vao);
		glGenBuffers(1, &batch_vbo);
		glGenBuffers(1, &batch_ebo);

		// 1. bind Vertex Array Object
		glBindVertexArray(batch_vao);

		// 2. copy our vertices array in a vertex buffer for OpenGL to use
		glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * BATCH_MAX_VERTICES, nullptr, GL_DYNAMIC_DRAW);

		u32* indices = (u32*) malloc(BATCH_MAX_INDICES * sizeof(u32));
		defer { free(indices); };

		u32 offset = 0;
		for (size_t i = 0; i < BATCH_MAX_INDICES; i += 6) {
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		// 3. copy our index array in a element buffer for OpenGL to use
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * BATCH_MAX_INDICES, indices, GL_STATIC_DRAW);

		// 4. then set the vertex attributes pointers
		set_vertex_attribs();

		glBindVertexArray(0);

		batch_vertices.data = (Vertex*) malloc(BATCH_MAX_VERTICES * sizeof(Vertex));
		batch_vertices.capacity = BATCH_MAX_VERTICES;

		// stub texture
		glGenTextures(1, &stub_texture);
		glBindTexture(GL_TEXTURE_2D, stub_texture);

		u32 pixel_data[1] = {0xffffffff};
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// 
	// Load shaders.
	// 
	{
		auto compile_shader = [&](GLenum type, const char* source) {
			u32 shader = glCreateShader(type);

			const char* sources[] = {source};
			glShaderSource(shader, ArrayLength(sources), sources, NULL);

			glCompileShader(shader);

			int success;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				char buf[512];
				glGetShaderInfoLog(shader, sizeof(buf), NULL, buf);
				log_error("Shader Compile Error: %s", buf);
			}

			return shader;
		};

		auto link_program = [&](u32 vertex_shader, u32 fragment_shader) {
			u32 program = glCreateProgram();

			glAttachShader(program, vertex_shader);
			glAttachShader(program, fragment_shader);

			glLinkProgram(program);

			int success;
			glGetProgramiv(program, GL_LINK_STATUS, &success);
			if (!success) {
				char buf[512];
				glGetProgramInfoLog(program, sizeof(buf), NULL, buf);
				log_error("Shader Link Error: %s", buf);
			}

			return program;
		};

		u32 texture_vert_shader = compile_shader(GL_VERTEX_SHADER, texture_vert_shader_src);
		defer { glDeleteShader(texture_vert_shader); };

		u32 texture_frag_shader = compile_shader(GL_FRAGMENT_SHADER, texture_frag_shader_src);
		defer { glDeleteShader(texture_frag_shader); };

		texture_shader = link_program(texture_vert_shader,texture_frag_shader);
	}
}

void deinit_renderer() {
	free(batch_vertices.data);
}

void break_batch() {
	if (batch_vertices.count == 0) {
		return;
	}

	Assert(current_mode != MODE_NONE);

	{
		glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, batch_vertices.count * sizeof(Vertex), batch_vertices.data);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	switch (current_mode) {
		case MODE_QUADS: {
			u32 program = texture_shader;

			glUseProgram(program);
			defer { glUseProgram(0); };

			mat4 MVP = (proj_mat * view_mat) * model_mat;

			int u_MVP = glGetUniformLocation(program, "u_MVP");
			glUniformMatrix4fv(u_MVP, 1, GL_FALSE, &MVP[0][0]);

			glBindTexture(GL_TEXTURE_2D, current_texture);
			defer { glBindTexture(GL_TEXTURE_2D, 0); };

			glBindVertexArray(batch_vao);
			defer { glBindVertexArray(0); };

			Assert(batch_vertices.count % 4 == 0);

			glDrawElements(GL_TRIANGLES, (GLsizei)batch_vertices.count / 4 * 6, GL_UNSIGNED_INT, 0);
			draw_calls++;
			max_batch = max(max_batch, batch_vertices.count);
			break;
		}

		case MODE_TRIANGLES: {
			u32 program = texture_shader;

			glUseProgram(program);
			defer { glUseProgram(0); };

			mat4 MVP = (proj_mat * view_mat) * model_mat;

			int u_MVP = glGetUniformLocation(program, "u_MVP");
			glUniformMatrix4fv(u_MVP, 1, GL_FALSE, &MVP[0][0]);

			glBindTexture(GL_TEXTURE_2D, stub_texture);
			defer { glBindTexture(GL_TEXTURE_2D, 0); };

			glBindVertexArray(batch_vao);
			defer { glBindVertexArray(0); };

			Assert(batch_vertices.count % 3 == 0);

			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)batch_vertices.count);
			draw_calls++;
			max_batch = max(max_batch, batch_vertices.count);
			break;
		}
	}

	batch_vertices.count = 0;
	current_texture = 0;
	current_mode = MODE_NONE;
}


void draw_texture(Texture t, Rect src,
				  vec2 pos, vec2 scale,
				  vec2 origin, float angle, vec4 color, glm::bvec2 flip) {
	if (src.w == 0 && src.h == 0) {
		src.w = t.width;
		src.h = t.height;
	}

	if (t.ID != current_texture || current_mode != MODE_QUADS) {
		break_batch();

		current_texture = t.ID;
		current_mode = MODE_QUADS;
	}

	{
		float x1 = -origin.x;
		float y1 = -origin.y;
		float x2 = src.w - origin.x;
		float y2 = src.h - origin.y;

		float u1 =  src.x          / (float)t.width;
		float v1 =  src.y          / (float)t.height;
		float u2 = (src.x + src.w) / (float)t.width;
		float v2 = (src.y + src.h) / (float)t.height;

		if (flip.x) {
			float temp = u1;
			u1 = u2;
			u2 = temp;
		}

		if (flip.y) {
			float temp = v1;
			v1 = v2;
			v2 = temp;
		}

		Vertex vertices[] = {
			{{x1, y1, 0.0f}, {}, color, {u1, v1}},
			{{x2, y1, 0.0f}, {}, color, {u2, v1}},
			{{x2, y2, 0.0f}, {}, color, {u2, v2}},
			{{x1, y2, 0.0f}, {}, color, {u1, v2}},
		};

		// Has to be in this order (learned it the hard way)
		mat4 model = glm::translate(mat4{1.0f}, {pos.x, pos.y, 0.0f});
		model = glm::rotate(model, glm::radians(-angle), {0.0f, 0.0f, 1.0f});
		model = glm::scale(model, {scale.x, scale.y, 1.0f});

		vertices[0].pos = model * vec4{vertices[0].pos, 1.0f};
		vertices[1].pos = model * vec4{vertices[1].pos, 1.0f};
		vertices[2].pos = model * vec4{vertices[2].pos, 1.0f};
		vertices[3].pos = model * vec4{vertices[3].pos, 1.0f};

		array_add(&batch_vertices, vertices[0]);
		array_add(&batch_vertices, vertices[1]);
		array_add(&batch_vertices, vertices[2]);
		array_add(&batch_vertices, vertices[3]);
	}
}

void draw_rectangle(Rectf rect, vec4 color) {
	Texture t = {stub_texture, 1, 1};
	vec2 pos = {rect.x, rect.y};
	vec2 scale = {rect.w, rect.h};
	draw_texture(t, {}, pos, scale, {}, 0, color);
}

void draw_rectangle(Rectf rect, vec2 scale,
					vec2 origin, float angle, vec4 color) {
	Texture t = {stub_texture, 1, 1};
	vec2 pos = {rect.x, rect.y};

	origin.x /= rect.w;
	origin.y /= rect.h;

	rect.w *= scale.x;
	rect.h *= scale.y;

	draw_texture(t, {}, pos, {rect.w, rect.h}, origin, angle, color);
}

void draw_triangle(vec2 p1, vec2 p2, vec2 p3, vec4 color) {
	if (current_mode != MODE_TRIANGLES) {
		break_batch();
		current_mode = MODE_TRIANGLES;
	}

	{
		Vertex vertices[] = {
			{{p1.x, p1.y, 0.0f}, {}, color, {}},
			{{p2.x, p2.y, 0.0f}, {}, color, {}},
			{{p3.x, p3.y, 0.0f}, {}, color, {}},
		};

		array_add(&batch_vertices, vertices[0]);
		array_add(&batch_vertices, vertices[1]);
		array_add(&batch_vertices, vertices[2]);
	}
}

void draw_circle(vec2 pos, float radius, vec4 color, int precision) {
	for (int i = 0; i < precision; i++) {
		float angle1 = (float)i       / (float)precision * 2.0f * glm::pi<float>();
		float angle2 = (float)(i + 1) / (float)precision * 2.0f * glm::pi<float>();
		vec2 p1 = pos + vec2{cosf(angle1), -sinf(angle1)} * radius;
		vec2 p2 = pos + vec2{cosf(angle2), -sinf(angle2)} * radius;
		draw_triangle(p1, p2, pos, color);
	}
}