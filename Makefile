CC = gcc
CFLAGS = -pthread -g -Wall -O2 -march=native -mtune=native

HDRS := $(wildcard *.h)
SRCS := $(wildcard *.c)
EXES := $(SRCS:.c=.out)

.PHONY: default all clean

default: $(EXES)
all: default

%.out: %.c $(HDRS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *.out
