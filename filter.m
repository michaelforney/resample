% SPDX-License-Identifier: Unlicense
L = 256;
N = 24;
beta = 8;
cutoff = 0.85;

pkg load signal;
w = kaiser(L * N + 1, beta);
h = int16(L * fir1(L * N, cutoff / L, w) * 0x7fff)(L *N / 2 + 1:end);
% Alternatively:
%h = int16(cutoff * sinc(cutoff * (0:1/L:N/2)) .* w'(L * N / 2 + 1:end) * 0x7fff);

d = [diff(h) 0];

for i = 1:length(h)
	fprintf("\t{%d, %d},\n", h(i), d(i))
endfor
