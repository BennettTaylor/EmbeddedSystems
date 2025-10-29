# Bennett Taylor betaylor

# Define variables
CC = gcc
LDFLAGS = -lm
TARGET = MyBitApp
SRCS = main.c bits.c mylist.c
OBJS = $(SRCS:.c=.o)

# Default rule
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rules for compiling source files
main.o: bits.h mylist.h
bits.o: bits.h
mylist.o: mylist.h

# Clean rule implementation
.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJS)
