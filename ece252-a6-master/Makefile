RM = rm -rf
CC = gcc
CFLAGS = -std=c99 -g -D_POSIX_C_SOURCE=200809L

default: all

all: bin server 

bin:
	mkdir -p bin/

server: rebel-fleet.c
	$(CC) $< $(CFLAGS) -pthread -o bin/rebel-fleet 

clean:
	$(RM) bin/

.PHONY: server clean
