CC = gcc
CFLAGS = -g -std=c99 -Wall -fsanitize=address,undefined -pthread

all: server

server: ttts.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^


clean:
	rm -f server *.o
