/* SPDX-License-Identifier: Unlicense */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "resample.h"

#define LEN(a) (sizeof(a) / sizeof((a)[0]))

static void
usage(void)
{
	fprintf(stderr, "usage: resample in-rate out-rate\n");
	exit(2);
}

int
main(int argc, char *argv[])
{
	struct resampler r;
	int16_t in[4096], out[4096];
	char *end;
	int done = 0;

	if (argc != 3)
		usage();

	resample_init(&r);
	r.in_rate = strtol(argv[1], &end, 0);
	if (*end)
		usage();
	r.out_rate = strtol(argv[2], &end, 0);
	if (*end)
		usage();
	r.in_stride = 1;
	r.out_stride = 1;

	while (!done) {
		r.in = in;
		if (feof(stdin)) {
			resample_eof(&r);
			done = 1;
		} else {
			r.in_frames = fread(in, sizeof(in[0]), LEN(in), stdin);
			if (ferror(stdin)) {
				perror("read");
				return 1;
			}
		}
		while (r.in_frames > 0) {
			r.out = out;
			r.out_frames = LEN(out);
			fwrite(out, resample(&r), sizeof(out[0]), stdout);
		}
	}
	fflush(stdout);
	if (ferror(stdout)) {
		perror("write");
		return 1;
	}
}
