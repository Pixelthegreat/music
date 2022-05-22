CC=cc
CFLAGS=-c `pkg-config --cflags gtk+-3.0 gstreamer-1.0`
LDFLAGS=`pkg-config --libs gtk+-3.0 gstreamer-1.0`
DESTDIR=/usr

all: music

music: main.o player.o
	$(CC) main.o player.o -o music $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) main.c

player.o: player.c player.h
	$(CC) $(CFLAGS) player.c

clean:
	rm *.o music

install:
	cp -v music $(DESTDIR)/bin/
	cp -v music.desktop /usr/share/applications/
