
CC = g++
CFLAGS = -O3 -lpthread

all: randtrack 

randtrack: list.h hash.h defs.h randtrack.cc
	$(CC) $(CFLAGS) randtrack.cc -o randtrack


clean:
	rm -f *.o randtrack randtrack_global_lock randtrack_tm randtrack_list_lock
