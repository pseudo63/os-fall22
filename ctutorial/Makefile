.PHONY: clean

CC=gcc
CFLAGS=
OBJS=list.o list_harness.o

list_harness: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o list_harness

list.o: list.c list.c

list_harness.o: list_harness.c list.h

clean:
	rm *.o