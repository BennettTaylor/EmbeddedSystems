# Define variables
CC = gcc
LDFLAGS = -lm
TARGET = myISS
SRCS = myISS.c
OBJS = $(SRCS:.c=.o)

# Default rule
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Clean rule implementation
.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJS)
