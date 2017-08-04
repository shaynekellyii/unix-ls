CC = gcc
CFLAGS = -g -Wall -Wextra -I -pthread
PROG = UnixLs 
OBJS = UnixLs.o

UnixLs: $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

UnixLs.o:
	$(CC) $(CFLAGS) -c UnixLs.c

clean:
	rm *.o UnixLs 
