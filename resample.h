struct resampler {
	const int16_t *in;
	int16_t *out;
	size_t in_frames, out_frames;
	unsigned in_stride, out_stride;
	unsigned long in_rate, out_rate;

	short *x, buf[2 * (2 * 12 * 8 + 1)];

	/* time since last input sample read, in units of input sampling period / (L * 2^16) */
	unsigned long t;
};

void resample_init(struct resampler *);
void resample_eof(struct resampler *);
size_t resample(struct resampler *);
