/* SPDX-License-Identifier: Unlicense */
/*
windowed-sinc resampler, using the technique described in
https://ccrma.stanford.edu/~jos/resample/
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "resample.h"

#define LEN(a) (sizeof(a) / sizeof((a)[0]))

enum {
	L = 256,
};

static const short filter[][2] = {
#include "filter.h"
};

enum {
	N = (LEN(filter) - 1) / L,
	B = (LEN(((struct resampler *)0)->buf) / 2 - 1) / 2,
};

void
resample_init(struct resampler *r)
{
	/* set t to initialize the buffer with the first B + 1 samples */
	r->t = L * (B + 1) << 16;
	r->x = r->buf + B;
	memset(r->buf, 0, sizeof(r->buf));
}

void
resample_eof(struct resampler *r)
{
	static const short zero[1];

	/* flush the buffer by reading B zeros */
	r->in = zero;
	r->in_frames = B;
	r->in_stride = 0;
}

static inline int16_t
h(unsigned long t)
{
	const int16_t *c = filter[t >> 16];
	return c[0] + (c[1] * (long)(t & 0xffff) >> 16);
}

size_t
resample(struct resampler *r)
{
	unsigned long f_in = r->in_rate, f_out = r->out_rate;
	unsigned long cutoff = f_in < f_out ? 0x10000 : f_out * 0x10000ull / f_in;
	unsigned long t = r->t, t_in = L << 16, t_out = (unsigned long long)t_in * f_in / f_out;
	unsigned long p, dt = t_in * cutoff >> 16, t_max = N * t_in;
	short *x = r->x, *end = r->buf + 3 * B + 1;
	size_t n = 0;
	long y;
	int i;

	for (;;) {
		for (; t >= t_in; t -= t_in, --r->in_frames) {
			if (r->in_frames == 0)
				goto done;
			if (x == end)
				x -= 2 * B + 1;
			x[-B] = x[B + 1] = *r->in;
			r->in += r->in_stride;
			++x;
		}
		for (; t < t_in; t += t_out, --r->out_frames) {
			if (r->out_frames == 0)
				goto done;
			y = 0;
			for (i = 0, p = t * cutoff >> 16; i <= B && p <= t_max; ++i, p += dt)
				y += x[-i] * h(p) >> 7;
			for (i = 1, p = (t_in - t) * cutoff >> 16; i <= B && p <= t_max; ++i, p += dt)
				y += x[i] * h(p) >> 7;
			/* round */
			if (y & 0x80)
				y += 0x7f;
			/* shift back and clamp to int16 range */
			y = (y >> 8) * (long)cutoff >> 16;
			if (y > INT16_MAX)
				y = INT16_MAX;
			else if (y < INT16_MIN)
				y = INT16_MIN;
			*r->out = y;
			r->out += r->out_stride;
			++n;
		}
	}
done:
	r->x = x;
	r->t = t;
	return n;
}
