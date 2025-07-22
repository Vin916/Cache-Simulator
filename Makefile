# Makefile for Cache Lab

CC = gcc
CFLAGS = -Wall -Werror -g

all: csim ctuner

csim: csim.c
	$(CC) $(CFLAGS) -o csim csim.c

ctuner: ctuner.c
	$(CC) $(CFLAGS) -o ctuner ctuner.c

clean:
	rm -f csim ctuner *.o
