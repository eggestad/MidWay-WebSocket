
CC=gcc

CFLAGS=-g `pkgconf --cflags json-c` `pkgconf --cflags glib-2.0`


OBJECTS=mwwsd.o protocol.o sender.o logging.o subscriptions.o pendingcalls.o


all: mwwsd

clean:
	rm -f *.o simple mwwsd *~

mwwsd: $(OBJECTS)
	$(CC) -o mwwsd -lwebsockets `pkgconf --libs json-c` `pkgconf --libs glib-2.0`  -lpthread -LMidWay $(OBJECTS)

simple.o: simple.c


mwwsd.o: mwwsd.c mwwsd.h

protocol.o: protocol.c   mwwsd.h

sender.o: sender.c mwwsd.h 

subscriptions.o: subscriptions.c mwwsd.h 

pendingcalls.o: pendingcalls.c mwwsd.h 

logging.o: logging.c mwwsd.h


