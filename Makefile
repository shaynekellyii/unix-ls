CC=gcc
CFLAGS=-g -Wall -Wextra -I.
PROG=UnixLs 
OBJS=UnixLs.o

UnixLs: $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

UnixLs.o: UnixLs.h

clean:
	rm *.o UnixLs 