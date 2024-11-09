# Compiler and Compile options.
CC=gcc
CFLAGS=-Wall -g

# Files
TARGET=client
SOURCES=client.c helpers.c buffer.c
OBJECTS=$(SOURCES:.c=.o)

# Default rule
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# To obtain object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# To remove generated files
clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
