.POSIX:
.PHONY: clean

CFLAGS+=-std=c99 -Wall -Wpedantic

all: resample

filter.h: filter.m
	octave filter.m >$@.tmp && mv $@.tmp $@

main.o: resample.h
resample.o: resample.h filter.h
resample: main.o resample.o

clean:
	rm -rf resample main.o resample.o
