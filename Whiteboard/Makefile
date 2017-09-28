CC=gcc
CCFLAGS=-g -std=c99 -D_BSD_SOURCE -D_POSIX_SOURCE -Wall
LIBS=-lreadline

TARGETS=server client4 client6

all: $(TARGETS)

server: server_main.o server.o common.o common.h server.h
	$(CC) $(CCFLAGS) -o $@ $^ $(LIBS)

server_main.o: server_main.c common.h server.h
	$(CC) $(CCFLAGS) -c $<

server.o: server.c common.h server.h
	$(CC) $(CCFLAGS) -c $<

client4: client4_main.o client.o common.o common.h client.h
	$(CC) $(CCFLAGS) -o $@ $^ $(LIBS)

client4_main.o: client4_main.c common.h client.h
	$(CC) $(CCFLAGS) -c $<

client6: client6_main.o client.o common.o common.h client.h
	$(CC) $(CCFLAGS) -o $@ $^ $(LIBS)

client6_main.o: client6_main.c common.h client.h
	$(CC) $(CCFLAGS) -c $<

client.o: client.c common.h client.h
	$(CC) $(CCFLAGS) -c $<

common.o: common.c common.h
	$(CC) $(CCFLAGS) -c $<

clean:
	rm -rf *.o $(TARGETS)
  
