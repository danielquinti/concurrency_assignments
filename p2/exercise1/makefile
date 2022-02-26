CC=gcc
CFLAGS=-Wall -pthread -g
LIBS=
OBJS1=probarcola.o options.o queue.o

PROGS= probarcola

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

probarcola: $(OBJS1)
	$(CC) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)
clean:
	rm -f $(PROGS) *.o *~
