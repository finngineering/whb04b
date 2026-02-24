CFLAGS = -Wall -Wextra -pedantic

all: firmware_readout

clean:
	rm firmware_readout
	rm *.o

# Check if math lib -lm is needed
firmware_readout: firmware_readout.o
	gcc -o firmware_readout firmware_readout.o -lusb-1.0 -lm

firmware_readout.o: firmware_readout.c
	gcc $(CFLAGS) -c -o firmware_readout.o firmware_readout.c