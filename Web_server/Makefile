CC=gcc
CCFLAGS=-g -std=c99 -D_POSIX_SOURCE -Wall -m32 -pthread

TARGETS=207httpd

all: $(TARGETS)

common.o: common.c common.h
	$(CC) $(CCFLAGS) -c $<

207httpd.o: 207httpd.c common.h
	$(CC) $(CCFLAGS) -c $<

207httpd: 207httpd.o common.o
	$(CC) $(CCFLAGS) -o $@ $^

clean:
	rm -f *.o $(TARGETS)
