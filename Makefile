CC=gcc
CFLAGS=-g3 -O2 -Wall -pthread
SRCS=src/main.c src/util.c src/pool.c src/http.c
OBJS=$(SRCS:.c=.o)

all: audioserv

audioserv: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	rm -rf $(OBJS)

clean:
	rm -rf $(OBJS) audioserv
