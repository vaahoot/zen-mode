CC = clang
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
TARGET = out/zen
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=out/%.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

out/%.o: src/%.c
	mkdir -p out
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f out/*
