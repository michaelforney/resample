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

/* evaluate the filter at time t, given in 16:16 fixed-point */
static inline int16_t
h(unsigned long t)
{
	const int16_t *c = filter[t >> 16];
	return c[0] + (c[1] * (long)(t & 0xffff) >> 16);
}

size_t
resample(struct resampler *r)
{
	/* resample ratio, Fout/Fin, as 16:16 fixed-point */
	unsigned long ratio = r->out_rate * 0x10000ull / r->in_rate;
	/* low-pass filter cutoff; 1 when upsampling, Fout/Fin when downsampling */
	unsigned long cutoff = ratio > 0x10000 ? 0x10000 : ratio;
	/* current time, input period, and output period */
	unsigned long t = r->t, t_in = L << 16, t_out = ((unsigned long long)t_in << 16) / ratio;
	/* filter position, step size, and maximum */
	unsigned long p, p_0, dp = L * cutoff, p_end = LEN(filter) << 16;
	short *x = r->x, *end = r->buf + 3 * B + 1;
	size_t n = 0, i;
	long y;

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
			p_0 = (unsigned long long)t * cutoff >> 16;
			/* left side of filter */
			for (i = 0, p = p_0; i <= B && p < p_end; ++i, p += dp)
				y += x[-i] * h(p) >> 7;
			/* right side of filter */
			for (i = 1, p = dp - p_0; i <= B && p < p_end; ++i, p += dp)
				y += x[i] * h(p) >> 7;
			/* scale back to int16 range */
			y = (((y + 0x80ll) >> 8) * (long)cutoff + 0x8000) >> 16;
			/* clamp to int16 range, if necessary */
			*r->out = y > 0x7fff ? 0x7fff : y < -0x8000 ? -0x8000 : y;
			r->out += r->out_stride;
			++n;
		}
	}
done:
	r->x = x;
	r->t = t;
	return n;
}
