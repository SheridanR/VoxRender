/*-------------------------------------------------------------------------------

	VOXRENDER
	File: voxrend.h
	Desc: contains some prototypes as well as various type definitions

	Copyright 2013 (c) Sheridan Rathbun, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "SDL.h"

// voxel structure
typedef struct voxel_t {
	long sizex, sizey, sizez;
	unsigned char *data;
	char palette[256][3];
} voxel_t;

// camera structure
typedef struct camera_t {
	double x, y, z;
	double ang;
} camera_t;
camera_t camera;

// various definitions
SDL_Surface *font8_bmp;
voxel_t model;
#define PI 3.1415926536
#define CLIPNEAR 5
#define CLIPFAR 2000
#define AVERAGEFRAMES 32
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
double t, ot = 0.0, frameval[AVERAGEFRAMES];
unsigned long cycles = 0;
long timesync = 0;
double fps = 0.0;

// min and max definitions
#define max(a,b) \
		({ typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a > _b ? _a : _b; })
#define min(a,b) \
		({ typeof (a) _a = (a); \
		typeof (b) _b = (b); \
		_a < _b ? _a : _b; })