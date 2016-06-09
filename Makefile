.PHONY: clean, mrproper
CC = gcc
CFLAGS = -g -Wall -Wextra

all: client

client.c: utils.h
utils.c: utils.h
network.c: network.h
network.h: conn_info.h utils.h
network_client.c: network_client.h
network_client.h: network.h

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

client: utils.o network.o network_client.o client.o
	$(CC) $(CFLAGS) -o $@ $+

clean:
	rm -f *.o core.*

mrproper: clean
	rm -f client
