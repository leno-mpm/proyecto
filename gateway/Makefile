CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = test_gateway
SOURCES = test_gateway.c gateway.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) gw1

.PHONY: all clean run
