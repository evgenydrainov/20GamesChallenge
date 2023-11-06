#pragma once

#include "common.h"
#include "stb_sprintf.h"
#include <SDL.h>
#include <stdlib.h>

static void* ecalloc(usize count, usize size) {
	void* result = calloc(count, size);
	if (!result) {
		ERROR("Out of memory.");
	}
	return result;
}
