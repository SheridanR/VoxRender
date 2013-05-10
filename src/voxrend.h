/*-------------------------------------------------------------------------------

	VOXRENDER
	File: voxrend.h
	Desc: contains some prototypes as well as various type definitions

	Copyright 2013 (c) Sheridan Rathbun, all rights reserved.
	See LICENSE.TXT for details.

-------------------------------------------------------------------------------*/

#include "SDL.h"

// voxel structure
typedef struct voxel_t {
	long sizex, sizey, sizez;
	unsigned char *data;
	char palette[256][3];
} voxel_t;

typedef struct voxbit_t {
	long sx, sy;
	int x1, x2, y1, y2;
	long d;
	Uint32 color;
	
	struct voxbit_t *next;
} voxbit_t;

SDL_Surface *screen;
SDL_Event event;
int xres = 640;
int yres = 480;
int fullscreen = 0;
long *zbuffer;
int mainloop = 1;
int keystatus[323];
int mousestatus[5];
int mousex, mousey;
#define AVERAGEFRAMES 32
double t, ot = 0.0, frameval[AVERAGEFRAMES];
unsigned long cycles = 0;
long timesync = 0;
double fps = 0.0;

#define max(a,b) \
		({ typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a > _b ? _a : _b; })
#define min(a,b) \
		({ typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a < _b ? _a : _b; })

#define PI 3.1415926536

// camera variables
double camx = 0;
double camy = 100;
double camz = 0;
double camang = 3.0*PI/2.0;
#define CLIPNEAR 5
#define CLIPFAR 2000

// various definitions
SDL_Surface *font8_bmp;
voxel_t model;