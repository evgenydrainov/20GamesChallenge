#pragma once

#include <stdint.h>
#include <stddef.h>

#define ArrayLength(a) (sizeof(a) / sizeof(*(a)))

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t    usize;
typedef ptrdiff_t ssize;

#define ERROR(...)																			\
	do {																					\
		char buf[1024];																		\
		stb_snprintf(buf, sizeof(buf), __VA_ARGS__);										\
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error.", buf, nullptr);		\
		exit(1);																			\
	} while (0)
