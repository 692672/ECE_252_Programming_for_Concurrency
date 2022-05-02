RM = rm -rf

CC = gcc
CFLAGS = -O2 -std=c99 -D_GNU_SOURCE
OMPFLAGS = -fopenmp -Wall

all: bin nqueens

bin:
	mkdir -p bin

nqueens: nqueens.c
	$(CC) $< $(CFLAGS) -o bin/$@ -pthread

clean:
	$(RM) bin

.PHONY: all
