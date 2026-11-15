CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = server
SOURCE = src/server.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
