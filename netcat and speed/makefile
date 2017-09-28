
CC=gcc
CCFLAGS=-g -std=c99 -D_BSD_SOURCE -Wall

TARGETS=netcatd netcat spdtestd spdtest

all: $(TARGETS)

netcatd: netcatd.o common.o common.h
	$(CC) $(CCFLAGS) -o $@ $^

netcatd.o: netcatd.c common.h
	$(CC) $(CCFLAGS) -c $<

netcat: netcat.o common.o common.h
	$(CC) $(CCFLAGS) -o $@ $^

netcat.o: netcat.c common.h
	$(CC) $(CCFLAGS) -c $<

spdtestd: spdtestd.o common.o common.h spdtest.h
	$(CC) $(CCFLAGS) -o $@ $^

spdtestd.o: spdtestd.c common.h spdtest.h
	$(CC) $(CCFLAGS) -c $<

spdtest: spdtest.o common.o common.h spdtest.h
	$(CC) $(CCFLAGS) -o $@ $^

spdtest.o: spdtest.c common.h spdtest.h
	$(CC) $(CCFLAGS) -c $<

common.o: common.c common.h
	$(CC) $(CCFLAGS) -c $<

clean:
	rm -rf *.o $(TARGETS)
