CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = test_broker
SOURCES = test_broker.c broker.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
