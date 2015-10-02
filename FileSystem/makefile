
OBJS = test.o mini_filesystem.o
CC = gcc
CFLAGS = -Wall -c
SOURCES = test.c mini_filesystem.c
HEADERS = mini_filesystem.h


all: $(OBJS)
	$(CC) $(SOURCES) -o test

clean:
	rm -rf *.o test
