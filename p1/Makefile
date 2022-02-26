
CC=gcc
CFLAGS=-Wall -pthread -g
LIBS=
OBJS1=swap1.o options.o
OBJS2=swap2.o options.o
OBJS3=swap3.o options.o
PROGS= swap1 swap2 swap3

all: $(PROGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

swap1: $(OBJS1)
	$(CC) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)
swap2: $(OBJS2)
	$(CC) $(CFLAGS) -o $@ $(OBJS2) $(LIBS)
swap3: $(OBJS3)
	$(CC) $(CFLAGS) -o $@ $(OBJS3) $(LIBS)
clean:
	rm -f $(PROGS) *.o *~
