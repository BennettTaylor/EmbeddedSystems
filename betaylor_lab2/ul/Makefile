# Define variables
CC = arm-linux-gnueabihf-gcc
TARGET = ktimer
SRCS = ktimer.c
OBJS = $(SRCS:.c=.o)

# Default rule
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -static

# Clean rule implementation
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
