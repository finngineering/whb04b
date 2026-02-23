CFLAGS = -Wall -Wextra -pedantic


all: whb04b

clean:
	rm whb04b
	rm *.o

# Check if math lib -lm is needed
whb04b: main.o
	gcc -o whb04b main.o -lusb-1.0 -lm

main.o: main.c
	gcc $(CFLAGS) -c -o main.o main.c