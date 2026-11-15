CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = server
SOURCE = src/server.c

all: $(TARGET) client

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

client: src/client.c
	$(CC) $(CFLAGS) src/client.c -o client

clean:
	rm -f $(TARGET) client

.PHONY: all clean
