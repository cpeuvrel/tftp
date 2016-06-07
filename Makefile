.PHONY: clean, mrproper
CC = gcc
CFLAGS = -g -Wall -Wextra

all: client

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

client: client.o
	$(CC) $(CFLAGS) -o $@ $+

clean:
	rm -f *.o core.*

mrproper: clean
	rm -f client
