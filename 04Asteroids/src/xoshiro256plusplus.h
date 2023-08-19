#pragma once

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include <stdint.h>

struct xoshiro256plusplus {
	uint64_t s[4] = {0x2AB4B67FC453AE6Dull, 0xC11449B610DF4A8Full, 0xEEBC88AF47D5688Aull, 0x1144BDD01AF56636ull};
	
	uint64_t next(void) {
		auto rotl = [](const uint64_t x, int k) -> uint64_t {
			return (x << k) | (x >> (64 - k));
		};

		const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

		const uint64_t t = s[1] << 17;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= t;

		s[3] = rotl(s[3], 45);

		return result;
	}

	// range [a, b)
	// i have no idea
	float range(float a, float b) {
		uint64_t x = next();
		double d = (x >> 11) * 0x1.0p-53;
		float f = (float) d;
		float result = a + (b - a) * f;
		if (result >= b) result = a;
		return result;
	}
};
