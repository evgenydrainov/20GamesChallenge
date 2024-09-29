#pragma once

#include "common.h"
#include "batch_renderer.h"

Texture load_texture_from_file(const char* fname,
							   int filter = GL_NEAREST, int wrap = GL_CLAMP_TO_BORDER);
