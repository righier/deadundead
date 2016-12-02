#pragma once

#include "utils.h"

#define INSTANCE_COUNT 1000

struct SpriteRenderer {
	unsigned int vao;
	unsigned int vbo;
	Texture *active_texture;
	Shader *active_shader;
	Mat3 active_matrix;
	Color active_color;
	float *buffer;
	int capacity;
	int size;
};

void init(SpriteRenderer *renderer, Texture *default_texture, Shader *default_shader);

void dispose(SpriteRenderer *renderer);

void bind(SpriteRenderer *renderer);

void flush(SpriteRenderer *renderer);

void draw(SpriteRenderer *renderer, Texture *texture, float x, float y, float w, float h);

void draw(SpriteRenderer *renderer, Texture *texture, float x, float y, float w, float h, float tx, float ty, float tw, float th);

void draw(SpriteRenderer *renderer, Font *font, char *text, float x, float y);

void set_color(SpriteRenderer *renderer, Color *color);

void set_shader(SpriteRenderer *renderer, Shader *shader);

void set_texture(SpriteRenderer *renderer, Texture *texture);

void set_matrix(SpriteRenderer *renderer, Mat3 *mat);