#################################################################################
#	
#	VOXRENDER
#	File: mingw32 makefile
#	Desc: The VoxRender makefile, mingw32 version
#	
#	Copyright 2013 (c) Sheridan Rathbun, all rights reserved.
#	See LICENSE for details.
#	
#################################################################################

CC=gcc
CFLAGS=-Wall -O3 -ffast-math -funroll-loops -malign-double -fstrict-aliasing
INCLUDE=-I/usr/include/SDL
LIB1=-L/usr/lib
LIB2=-L/usr/local/lib

all: voxrender clean
	
voxrender: voxrend.o
	$(CC) $(CFLAGS) voxrend.o -o voxrender $(LIB1) -lmingw32 $(LIB2) -lSDLmain -lSDL -mwindows -lpthread
	
voxrend.o:
	$(CC) $(CFLAGS) -c src/voxrend.c -o voxrend.o
	
clean:
	rm *.o