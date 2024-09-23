#pragma once


// -----------------------------------------------------------
//                 SECTION: Common types.
// -----------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <stdarg.h>   // for va_list
#include <string.h>   // for memcpy

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef ptrdiff_t ssize_t;

struct Rect {
	int x;
	int y;
	int w;
	int h;
};

struct Rectf {
	float x;
	float y;
	float w;
	float h;
};

#define ArrayLength(a) (sizeof(a) / sizeof(a[0]))

#define Kilobytes(x) ((size_t)((x) * (size_t)1024))
#define Megabytes(x) Kilobytes((x) * (size_t)1024)
#define Gigabytes(x) Megabytes((x) * (size_t)1024)

// 
// Logging and assert
// 

#define log_info(fmt, ...)  printf(fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#ifdef NDEBUG
#define Assert(expr) while (!(expr)) { log_error("ASSERTION FAILED: " #expr); exit(1); }
#else
#define Assert(expr) while (!(expr)) __debugbreak()
#endif

// 
// For loop
// 

#define For(it, arr)    for (auto it = arr.begin(); it != arr.end(); it++)
#define Remove(it, arr) (it = array_remove(&arr, it), it--)

#define Repeat(n)       for (int CONCAT(_i_, __LINE__) = (int)(n); CONCAT(_i_, __LINE__)--;)

#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)

// 
// Defer statement
// Stolen from Jonathan Blow's microsoft_craziness.h
// 

template<typename T>
struct ExitScope {
    T lambda;
    ExitScope(T lambda):lambda(lambda){}
    ~ExitScope(){lambda();}
    ExitScope(const ExitScope&);
  private:
    ExitScope& operator =(const ExitScope&);
};
 
class ExitScopeHelp {
  public:
    template<typename T>
        ExitScope<T> operator+(T t){ return t;}
};

#define defer const auto& CONCAT(_defer__, __LINE__) = ExitScopeHelp() + [&]()

// 
// Automatically convert an enum to a string.
// 

#define GENERATE_ENUM(x) x,
#define GENERATE_STRING(x) #x,


// 
// @Todo: The "Get##Type##Name" function is static, so I think the "names" array is
// copied a bunch of times.
// 
#define DEFINE_NAMED_ENUM(Type, List) \
enum Type { List(GENERATE_ENUM) }; \
static const char* Get##Type##Name(Type val) { \
	static const char* names[] = { List(GENERATE_STRING) }; \
	Assert(val >= 0); \
	Assert(val < ArrayLength(names)); \
	return names[val]; \
}

// 
// Human-readable printing for filesizes.
// 

#define Size_Fmt "%.7g %s"
#define Size_Arg(size) format_size_float(size), format_size_string(size)

static float format_size_float(size_t bytes) {
	float result = (float)bytes;
	if (bytes >= 1024) {
		result /= 1024.0f;
		bytes /= 1024;
		if (bytes >= 1024) {
			result /= 1024.0f;
			bytes /= 1024;
			if (bytes >= 1024) {
				result /= 1024.0f;
				bytes /= 1024;
			}
		}
	}
	result = floorf(result * 100.0f) / 100.0f;
	return result;
}

static const char* format_size_string(size_t bytes) {
	const char* result = "bytes";
	if (bytes >= 1024) {
		result = "KB";
		bytes /= 1024;
		if (bytes >= 1024) {
			result = "MB";
			bytes /= 1024;
			if (bytes >= 1024) {
				result = "GB";
				bytes /= 1024;
			}
		}
	}
	return result;
}



// --------------------------------------------------------
//                  SECTION: Math
// --------------------------------------------------------



// 
// Cirno's Perfect Math Library
// 

template <typename T>
static T min(T a, T b) {
	return (a < b) ? a : b;
}

template <typename T>
static T max(T a, T b) {
	return (a > b) ? a : b;
}

template <typename T>
static T clamp(T a, T mn, T mx) {
	return max(min(a, mx), mn);
}

template <typename T>
static void Clamp(T* a, T mn, T mx) {
	*a = clamp(*a, mn, mx);
}

template <typename T>
static T lerp(T a, T b, float f) {
	return a + (b - a) * f;
}

static void normalize0(float& x, float& y) {
	float length = sqrtf(x * x + y * y);
	if (length != 0) {
		x /= length;
		y /= length;
	} else {
		x = 0;
		y = 0;
	}
}

template <typename T>
static T approach(T start, T end, T shift) {
	return start + clamp(end - start, -shift, shift);
}

#ifndef PI
#define PI 3.14159265359f
#endif

static float to_degrees(float rad) { return rad / PI * 180.0f; }
static float to_radians(float deg) { return deg / 180.0f * PI; }

static float dsin(float deg) { return sinf(to_radians(deg)); }
static float dcos(float deg) { return cosf(to_radians(deg)); }

static float point_distance(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	return sqrtf(dx * dx + dy * dy);
}

static float point_direction(float x1, float y1, float x2, float y2) {
	return to_degrees(atan2f(y1 - y2, x2 - x1));
}

static bool circle_vs_circle(float x1, float y1, float r1, float x2, float y2, float r2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float r = r1 + r2;
	return (dx * dx + dy * dy) < (r * r);
}

static bool circle_vs_rotated_rect(float circle_x, float circle_y, float circle_radius,
								   float rect_center_x, float rect_center_y, float rect_w, float rect_h, float rect_dir) {
	float dx = circle_x - rect_center_x;
	float dy = circle_y - rect_center_y;

	float x_rotated = rect_center_x - (dx * dsin(rect_dir)) - (dy * dcos(rect_dir));
	float y_rotated = rect_center_y + (dx * dcos(rect_dir)) - (dy * dsin(rect_dir));

	float x_closest = clamp(x_rotated, rect_center_x - rect_w / 2.0f, rect_center_x + rect_w / 2.0f);
	float y_closest = clamp(y_rotated, rect_center_y - rect_h / 2.0f, rect_center_y + rect_h / 2.0f);

	dx = x_closest - x_rotated;
	dy = y_closest - y_rotated;

	return (dx * dx + dy * dy) < (circle_radius * circle_radius);
}

static float lengthdir_x(float len, float dir) { return  dcos(dir) * len; }
static float lengthdir_y(float len, float dir) { return -dsin(dir) * len; }

static float wrapf(float a, float b) {
	return fmodf((fmodf(a, b) + b), b);
}

template <typename T>
static T wrap(T a, T b) {
	return ((a % b) + b) % b;
}



// ----------------------------------------------------
//               SECTION: Memory Arena
// ----------------------------------------------------



// 
// Stolen from https://bytesbeneath.com/p/the-arena-custom-memory-allocators
// 

#define is_power_of_two(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

static uintptr_t align_forward(uintptr_t ptr, size_t align) {
	Assert(is_power_of_two(align));
	return (ptr + (align - 1)) & ~(align - 1);
}

struct Arena {
	static constexpr size_t DEFAULT_ALIGNMENT = sizeof(void*);

	u8*    data;
	size_t count;
	size_t capacity;
};

static Arena arena_create(size_t capacity) {
	Arena a = {};
	a.data     = (u8*) malloc(capacity);
	a.capacity = capacity;

	Assert(a.data);

	return a;
}

static void arena_destroy(Arena* a) {
	free(a->data);
	*a = {};
}

static u8* arena_push(Arena* a, size_t size, size_t alignment = Arena::DEFAULT_ALIGNMENT) {
	uintptr_t curr_ptr = (uintptr_t)a->data + (uintptr_t)a->count;
	uintptr_t offset = align_forward(curr_ptr, alignment);
	offset -= (uintptr_t)a->data;

	Assert(offset + size <= a->capacity && "Arena out of memory");

	u8* ptr = a->data + offset;
	a->count = offset + size;

	return ptr;
}

static Arena arena_create_from_arena(Arena* arena, size_t capacity) {
	Arena a = {};
	a.data     = arena_push(arena, capacity);
	a.capacity = capacity;

	return a;
}



// ----------------------------------------------------
//                SECTION: Array Type
// ----------------------------------------------------



// "Array view"
template <typename T>
struct array {
	T*     data;
	size_t count;

	T& operator[](size_t i) { Assert(i < count); return data[i]; }

	T* begin() { return data; }
	T* end()   { return data + count; }
};


// 
// A dynamic array that doesn't grow when it runs out of capacity, but
// replaces the last element.
// 
// @Todo: Come up with a cleanup strategy
// 
template <typename T>
struct dynamic_array_cap {
	T*     data;
	size_t count;
	size_t capacity;

	T& operator[](size_t i) { Assert(i < count); return data[i]; }

	T* begin() { return data; }
	T* end()   { return data + count; }
};

template <typename T>
static dynamic_array_cap<T> dynamic_array_cap_from_arena(Arena* a, size_t capacity) {
	dynamic_array_cap<T> arr = {};
	arr.data      = (T*) arena_push(a, capacity * sizeof(T));
	arr.capacity = capacity;

	return arr;
}

template <typename T>
static T* array_add(dynamic_array_cap<T>* arr, const T& val) {
	Assert(arr->count <= arr->capacity);

	if (arr->count == arr->capacity) {
		arr->count--;
	}

	arr->data[arr->count] = val;
	T* result = &arr->data[arr->count];
	arr->count++;

	return result;
}

template <typename T>
static T* array_remove(dynamic_array_cap<T>* arr, T* it) {
	Assert(it >= arr->begin());
	Assert(it <  arr->end());

	for (T* i = it; i < arr->end() - 1; i++) {
		*i = *(i + 1);
	}
	arr->count--;

	return it;
}

template <typename T>
static T* array_insert(dynamic_array_cap<T>* arr, size_t index, const T& val) {
	Assert(arr->count <= arr->capacity);

	//Assert(index >= 0);
	Assert(index <= arr->count);

	if (arr->count == arr->capacity) {
		arr->count--;
	}

	index = min(index, arr->capacity - 1);

	for (size_t i = arr->count; i-- > index;) {
		arr->data[i + 1] = arr->data[i];
	}

	T* result = &(arr->data[index] = val);

	arr->count++;

	return result;
}


// -----------------------------------------------
//             SECTION: String Type
// -----------------------------------------------


#define Str_Fmt      "%.*s"
#define Str_Arg(str) (unsigned int)(str).count, (str).data

struct string {
	char*  data;
	size_t count;

	string() = default;

	string(char* data, size_t count) : data(data), count(count) {}

	template <size_t N>
	string(const char (&arr)[N]) : data((char*) &arr[0]), count(N - 1) {}

	string(dynamic_array_cap<char> arr) : data(arr.data), count(arr.count) {}

	char& operator[](size_t i)       { Assert(i < count); return data[i]; }
	char  operator[](size_t i) const { Assert(i < count); return data[i]; }

	bool operator==(const string& other) {
		if (count != other.count) {
			return false;
		}

		for (size_t i = 0; i < count; i++) {
			if (data[i] != other[i]) {
				return false;
			}
		}

		return true;
	}
};

static bool is_whitespace(char ch) {
	return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

static bool is_numeric(char ch) {
	return ch >= '0' && ch <= '9';
}

static void advance(string* str, size_t i = 1) {
	i = min(i, str->count);
	str->count -= i;
	str->data  += i;
}

static void eat_whitespace(string* str) {
	while (str->count > 0 && is_whitespace(*str->data)) {
		advance(str);
	}
}

static string eat_non_whitespace(string* str) {
	string result = {str->data, 0};

	while (str->count > 0 && !is_whitespace(*str->data)) {
		advance(str);
		result.count++;
	}

	return result;
}

static string eat_numeric(string* str) {
	string result = {str->data, 0};

	while (str->count > 0 && is_numeric(*str->data)) {
		advance(str);
		result.count++;
	}

	return result;
}

static string eat_line(string* str) {
	string result = {str->data, 0};

	while (str->count > 0 && *str->data != '\n') {
		advance(str);
		result.count++;
	}

	// Skip newline character.
	if (str->count > 0 && *str->data == '\n') {
		advance(str);
	}

	return result;
}

static u32 string_to_u32(string str, bool* done = nullptr) {
	if (str.count == 0) {
		if (done) *done = false;
		return 0;
	}

	u32 result = 0;

	for (size_t i = 0; i < str.count; i++) {
		if (!is_numeric(str[i])) {
			if (done) *done = false;
			return 0;
		}

		result *= 10;
		result += str[i] - '0';
	}

	if (done) *done = true;
	return result;
}

static float string_to_f32(string str, bool* done = nullptr) {
	if (str.count == 0) {
		if (done) *done = false;
		return 0;
	}

	float negative = 1;

	while (str.count > 0 && *str.data == '-') {
		negative = -negative;
		advance(&str);
	}

	float result = 0;

	while (str.count > 0 && *str.data != '.') {
		if (!is_numeric(*str.data)) {
			if (done) *done = false;
			return 0;
		}

		result *= 10;
		result += *str.data - '0';

		advance(&str);
	}

	if (str.count > 0 && *str.data == '.') {
		advance(&str);

		float f = 0.1f;

		while (str.count > 0) {
			if (!is_numeric(*str.data)) {
				if (done) *done = false;
				return 0;
			}

			result += (*str.data - '0') * f;
			f /= 10;

			advance(&str);
		}
	}

	if (done) *done = true;
	return result * negative;
}

template <size_t N>
static string Sprintf(char (&buf)[N], const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);

	static_assert(N < (size_t)INT_MAX, "");

	int result = stb_vsnprintf(buf, (int)N, fmt, va);
	va_end(va);

	Assert(result > 0);
	return {buf, (size_t)result};
}


// You have to free() the C string
static char* to_c_string(string str) {
	char* c_str = (char*) malloc(str.count + 1);
	Assert(c_str);
	memcpy(c_str, str.data, str.count);
	c_str[str.count] = 0;
	return c_str;
}


// You have to free() string.data
static string copy_string(string str) {
	if (str.count == 0) return {};

	string result;
	result.data  = (char*) malloc(str.count);
	result.count = str.count;

	Assert(result.data);
	memcpy(result.data, str.data, str.count);

	return result;
}


