CC = gcc
CFLAGS = -g -Wall -Wextra -I -pthread
PROG = UnixLs
OBJS = UnixLs.o

run: $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm *.o UnixLs
