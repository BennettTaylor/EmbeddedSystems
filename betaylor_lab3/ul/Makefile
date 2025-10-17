CC = arm-linux-gnueabihf-gcc
CFLAGS = -std=c99 -Wall -I. -static -O0
DEPS = 
OBJ = ktimer.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ktimer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean:
	rm *.o ktimer
