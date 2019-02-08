
CC=gcc

CFLAGS=-g `pkgconf --cflags json-c`


OBJECTS=mwwsd.o protocol.o


all: mwwsd

clean:
	rm -f *.o simple mwwsd *~

mwwsd: $(OBJECTS)
	$(CC) -o mwwsd -lwebsockets `pkgconf --libs json-c` -lpthread -LMidWay $(OBJECTS)

simple.o: simple.c


mwwsd.o: mwwsd.c mwwsd.h

protocol.o: protocol.c   mwwsd.h
