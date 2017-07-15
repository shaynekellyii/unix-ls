CC = gcc
CFLAGS = -g -Wall -Wextra -I -pthread
PROG = UnixLs 
OBJS = UnixLs.o list.o

UnixLs: $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

list.o: list.c
	$(CC) $(CFLAGS) -c list.c

UnixLs.o: list.h
	$(CC) $(CFLAGS) -c UnixLs.c

clean:
	rm *.o UnixLs 
