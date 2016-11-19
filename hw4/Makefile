
CC = g++
CFLAGS = -O3 -lpthread
TMFLAGS = -fgnu-tm

all: randtrack randtrack_global_lock randtrack_tm randtrack_list_lock randtrack_element_lock

randtrack: list.h hash.h defs.h randtrack.cc
	$(CC) $(CFLAGS) randtrack.cc -o randtrack

randtrack_global_lock: list.h hash.h defs.h randtrack_global_lock.cc
	$(CC) $(CFLAGS) randtrack_global_lock.cc -o randtrack_global_lock

randtrack_list_lock: list.h hash.h defs.h randtrack_list_lock.cc
	$(CC) $(CFLAGS) randtrack_list_lock.cc -o randtrack_list_lock

randtrack_element_lock: list.h hash.h defs.h randtrack_element_lock.cc
	$(CC) $(CFLAGS) randtrack_element_lock.cc -o randtrack_element_lock

randtrack_tm: list.h hash.h defs.h randtrack_tm.cc
	$(CC) $(CFLAGS) $(TMFLAGS) randtrack_tm.cc -o randtrack_tm

clean:
	rm -f *.o randtrack randtrack_global_lock randtrack_tm randtrack_list_lock randtrack_element_lock
