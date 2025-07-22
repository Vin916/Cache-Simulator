# Makefile for Cache Lab

CC = gcc
CFLAGS = -Wall -Werror -g

all: cache-sim cache-optimizer

cache-sim: cache-sim.c
	$(CC) $(CFLAGS) -o cache-sim cache-sim.c

cache-optimizer: cache-optimizer
	$(CC) $(CFLAGS) -o cache-optimizer cache-optimizer.c

clean:
	rm -f cache-sim ctuner *.o
