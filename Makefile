CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = main
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: clean all
	./$(TARGET)

.PHONY: all clean run
