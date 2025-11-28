CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = test_freertos
SOURCES = test_freertos.c mi_freertos.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	gdb ./$(TARGET)

.PHONY: all clean run debug
