.PHONY: clean

CC=gcc
CFLAGS=

list_harness: list.o
	$(CC) $(CFLAGS) list.o -o list_harness

list.o: list.c list.c

clean:
	rm *.o