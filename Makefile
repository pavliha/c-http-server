CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = build/main
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -rf build

run: clean all
	$(TARGET)

.PHONY: all clean run
