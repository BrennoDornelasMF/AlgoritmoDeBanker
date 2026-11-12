CC = cc
CPPFLAGS =
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS =
LDLIBS =

.PHONY: all clean

all: banker

banker: banker.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) banker