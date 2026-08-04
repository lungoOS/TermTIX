
TARGET = termtix

CC = gcc

CFLAGS = -O2 -Wall $(shell pkg-config --cflags gtk+-3.0 vte-2.91)

LIBS = $(shell pkg-config --libs gtk+-3.0 vte-2.91)

SRCS = main.c terminal.c terminal.h config.c config.h

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
