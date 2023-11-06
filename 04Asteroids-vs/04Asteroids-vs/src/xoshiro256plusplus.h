#pragma once

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include "common.h"
#include <stdint.h>

struct xoshiro256plusplus {
	u64 s[4] = {0x2AB4B67FC453AE6Dull, 0xC11449B610DF4A8Full, 0xEEBC88AF47D5688Aull, 0x1144BDD01AF56636ull};
};

inline u64 random_next(xoshiro256plusplus* rng) {
	auto rotl = [](const u64 x, int k) -> u64 {
		return (x << k) | (x >> (64 - k));
	};

	u64* s = rng->s;

	const u64 result = rotl(s[0] + s[3], 23) + s[0];

	const u64 t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}

// [a, b)
inline float random_range(xoshiro256plusplus* rng, float a, float b) {
	u64 x = random_next(rng);
	float f = ((x >> 32) >> 8) * 0x1.0p-24f;
	float result = a + (b - a) * f;
	if (result >= b) result = a;
	return result;
}

// [a, b]
inline int random_range(xoshiro256plusplus* rng, int a, int b) {
	// int x = random_next(rng) >> 33;
	// int result = a + x % (b - a + 1);
	// return result;

	u64 x = random_next(rng);
	return a + x % (b - a + 1);
}

inline float random_chance(xoshiro256plusplus* rng, float percent) {
	float x = random_range(rng, 0.0f, 100.0f);
	return x < percent;
}
