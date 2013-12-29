/*-------------------------------------------------------------------------------

	VOXRENDER
	File: voxrend.c
	Desc: contains main code for voxrender project

	Copyright 2013 (c) Sheridan Rathbun, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "voxrend.h"

void ReceiveInput(void) {
	int speed;
	double d;
	int j;

	// calculate app rate
	t = SDL_GetTicks();
	timesync = t-ot;
	ot = t;
	
	// calculate fps
	if( timesync != 0 )
		frameval[cycles&(AVERAGEFRAMES-1)] = 1.0/timesync;
	else
		frameval[cycles&(AVERAGEFRAMES-1)] = 1.0;
	d = frameval[0];
	for(j=1;j<AVERAGEFRAMES;j++)
		d += frameval[j];
	fps = d/AVERAGEFRAMES*1000;
	
	// get input
	mousex = 0;
	mousey = 0;
	while( SDL_PollEvent(&event) ) { // poll SDL events
		// Global events
		switch( event.type ) {
			case SDL_QUIT: // if SDL receives the shutdown signal
				mainloop = 0;
				break;
			case SDL_KEYDOWN: // if a key is pressed...
				keystatus[event.key.keysym.sym] = 1; // set this key's index to 1
				break;
			case SDL_KEYUP: // if a key is unpressed...
				keystatus[event.key.keysym.sym] = 0; // set this key's index to 0
				break;
			case SDL_MOUSEBUTTONDOWN: // if a mouse button is pressed...
				mousestatus[event.button.button] = 1; // set this mouse button to 1
				break;
			case SDL_MOUSEBUTTONUP: // if a mouse button is released...
				mousestatus[event.button.button] = 0; // set this mouse button to 0
				break;
			case SDL_MOUSEMOTION: // if the mouse is moved...
				mousex = event.motion.xrel;
				mousey = event.motion.yrel;
				break;
		}
	}
	
	if( keystatus[SDLK_ESCAPE] )
		mainloop=0;
		
	// move camera
	speed = 1+keystatus[SDLK_LSHIFT];
	camera.x += (keystatus[SDLK_UP]-keystatus[SDLK_DOWN])*cos(camera.ang)*timesync*.125*speed;
	camera.y += (keystatus[SDLK_UP]-keystatus[SDLK_DOWN])*sin(camera.ang)*timesync*.125*speed;
	camera.z += (keystatus[SDLK_PAGEDOWN]-keystatus[SDLK_PAGEUP])*timesync*.25*speed;
	camera.ang += (keystatus[SDLK_RIGHT]-keystatus[SDLK_LEFT])*timesync*.002;
	while( camera.ang > PI*2 )
		camera.ang -= PI*2;
	while( camera.ang < 0 )
		camera.ang += PI*2;
}

void PrintText( SDL_Surface *font_bmp, int x, int y, char *fmt, ... ) {
	int c;
	int numbytes;
	char str[100];
	va_list argptr;
	SDL_Rect src, dest, odest;
	
	// format the string
	va_start( argptr, fmt );
	numbytes = vsprintf( str, fmt, argptr );
	
	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w/16; src.w = font_bmp->w/16;
	dest.h = font_bmp->h/16; src.h = font_bmp->h/16;
	
	// print the characters in the string
	for( c=0; c<numbytes; c++ ) {
		// edge of the screen prompts an automatic newline
		if( xres-dest.x < src.w ) {
			dest.x = x;
			dest.y += src.h;
		}
		
		src.x = (str[c]*src.w)%font_bmp->w;
		src.y = floor((str[c]*src.w)/font_bmp->w)*src.h;
		odest.x=dest.x; odest.y=dest.y;
		SDL_BlitSurface( font_bmp, &src, screen, &dest );
		dest.x=odest.x; dest.y=odest.y;
		switch( str[c] ) {
			case 10: // line feed
				dest.x = x;
				dest.y += src.h;
				break;
			default:
				dest.x += src.w; // move over one character
				break;
		}
	}
	va_end( argptr );
}

void DrawVoxel( voxel_t *model, long posx, long posy, long posz, double yaw, double pitch, double roll ) {
	double d;
	double dx, dy, dz;
	double ax, ay;
	double cosang, sinang;
	double cosyaw, sinyaw, cospitch, sinpitch, cosroll, sinroll;
	double offX, offY, offZ;
	double perspective;

	// drawing variables
	int hx, hy, hz;
	int voxX, voxY, voxZ;
	int x, y;
	int sx, sy;
	int x1, x2, y1, y2;
	int screenindex;
	int index;
	int zoffset;
	int bitsize;
	double sprsize;
	Uint8 *p;
	Uint32 color;
	
	// model angles
	cosyaw = cos(yaw);
	sinyaw = sin(yaw);
	cospitch = cos(pitch);
	sinpitch = sin(pitch);
	cosroll = cos(roll);
	sinroll = sin(roll);
	
	// viewport variables
	cosang = cos(camera.ang);
	sinang = sin(camera.ang);
	hx = xres>>1; hy = yres>>1; hz = hx;
	sprsize = 128.0*((double)xres/320.0);
	perspective = (1.0/xres/320.0)*(PI/2.0);

	// generate a list of voxbits to be drawn
	for( voxX=0; voxX<model->sizex; voxX++ ) {
		for( voxY=0; voxY<model->sizey; voxY++ ) {
			for( voxZ=0; voxZ<model->sizez; voxZ++ ) {
				// get the bit color
				index = voxZ+voxY*model->sizez+voxX*model->sizez*model->sizey;
				if( model->data[index] != 255 && model->data[index] )
					color = SDL_MapRGB( screen->format, model->palette[model->data[index]][0]<<2, model->palette[model->data[index]][1]<<2, model->palette[model->data[index]][2]<<2 );
				else
					continue;
					
				// calculate model offsets
				offX=(voxX - model->sizex/2);
				offY=(model->sizey/2 - voxY);
				offZ=(voxZ - model->sizez/2);
				
				// first, calculate the position of the voxbit relative to the camera (performing rotation)
				dx = posx - camera.x + (offX*cospitch*cosyaw) + (offY*cosroll*sinyaw + offY*sinroll*sinpitch*cosyaw) + offZ*sinroll*sinyaw - offZ*cosroll*sinpitch*cosyaw;
				dy = camera.y - posy - (offX*cospitch*sinyaw) + (offY*cosroll*cosyaw - offY*sinroll*sinpitch*sinyaw) + offZ*sinroll*cosyaw + offZ*cosroll*sinpitch*sinyaw;
				dz = posz - camera.z + (offX*sinpitch) - (offY*sinroll*cospitch) + (offZ*cosroll*cospitch);
				
				// get the onscreen direction to the voxbit
				ax = (dx*sinang)+(dy*cosang);
				ay = (dx*cosang)-(dy*sinang);
				if( ay <= 0 ) continue; // bit is outside the view frustrum
				
				// calculate distance
				d = sqrt(dx*dx + dy*dy);
				
				// compute and store final information about the voxbit
				sx = (ax*(hx/ay)*-1)+hx; // onscreen position x
				d = min(max(d*cos((sx-hx)*perspective),d*cos(PI/4)),d); // correct fishbowl effect
				if( d < CLIPNEAR || d > CLIPFAR )
					continue;
				sy = hy+((hz/d)*dz); // onscreen position y
				
				bitsize = 1.0/d*sprsize;
				x1 = max( sx-bitsize-1, 0 );
				x2 = min( sx+bitsize+1, xres );
				y1 = max( sy-bitsize-1, 0 );
				y2 = min( sy+bitsize+1, yres );
				if( x1>=xres || x2<0 || y1>=yres || y2<0 )
					continue;
				
				// draw the voxbit
				for( x=x1; x<x2; x++ ) {
					zoffset=x*yres;
					p = (Uint8 *)screen->pixels + x * screen->format->BytesPerPixel;
					screenindex = y1*screen->pitch;
					for( y=y1; y<y2; y++ ) {
						index=y+zoffset;
						if( d <= zbuffer[index] || !zbuffer[index] ) {
							*(Uint32 *)((Uint8 *)p + screenindex)=color;
							zbuffer[index]=d-0.01;
						}
						screenindex += screen->pitch;
					}
				}
			}
		}
	}
}

int main(int argc, char **argv ) {
	char *filename = NULL;
	char temp[10];
	int file;
	int a;
	double modelang=0;
	long vx=0;
	long vy=0;
	long vz=0;
	
	// set camera position
	camera.x = 0;
	camera.y = 100;
	camera.z = 0;
	camera.ang = 3*PI/2;

	// load a voxel model
	if( argc>1 ) {
		for(a=1; a<argc; a++) {
			if( argv[a] != NULL ) {
				if( !strcmp(argv[a], "-fullscreen") ) {
					fullscreen = 1;
				}
				else if( !strncmp(argv[a], "-size=", 6) ) {
					strncpy(temp,argv[a]+6,strcspn(argv[a]+6,"x"));
					xres = max(320,atoi(temp));
					yres = max(200,atoi(argv[a]+6+strcspn(argv[a]+6,"x")+1));
				}
				else {
					filename = (char *) malloc(sizeof(char)*strlen(argv[a])+4);
					strcpy(filename,argv[a]);
					if( strstr(filename,".vox") == NULL )
						strcat(filename,".vox");
					if((file = open(filename,O_RDONLY))==-1) {
						printf("Failed to open specified file.");
						return 1;
					}
				}
			}
		}
	}
	if( filename == NULL ) {
		printf("Usage: voxrender FILE\n");
		printf("Ex: voxrender desklamp.vox\n");
		return 1;
	}
	read(file,&model.sizex,4);
	read(file,&model.sizey,4);
	read(file,&model.sizez,4);
	model.data = (unsigned char *) malloc(model.sizex*model.sizey*model.sizez);
	read(file,model.data,model.sizex*model.sizey*model.sizez);
	read(file,model.palette,768);
	close(file);
	
	// load the font
	font8_bmp = SDL_LoadBMP("8font.bmp");
	SDL_SetColorKey( font8_bmp, SDL_SRCCOLORKEY, SDL_MapRGB( font8_bmp->format, 255, 0, 255 ) );
	
	// initialize sdl
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) == -1 ) {
		printf("Could not initialize SDL. Aborting...\n\n");
		return 1;
	}
	if( fullscreen ) {
		screen = SDL_SetVideoMode( xres, yres, 32, SDL_HWSURFACE | SDL_FULLSCREEN );
		SDL_ShowCursor(0);
	}
	else
		screen = SDL_SetVideoMode( xres, yres, 32, SDL_HWSURFACE );
	SDL_WM_SetCaption( "VoxRender", 0 );
	zbuffer = (long *) malloc(xres*yres*sizeof(long));
	
	// main loop
	while(mainloop) {
		// receive input + move camera
		ReceiveInput();
		
		// rotate the model
		modelang += timesync*.0025;
		while( modelang > PI*2 )
			modelang -= PI*2;
		if(keystatus[SDLK_SPACE]) {
			vx = camera.x;
			vy = camera.y;
			vz = camera.z;
			modelang = camera.ang;
		}
		
		// rendering
		memset( zbuffer, 0, xres*yres*sizeof(long) );
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,50,50,150)); // wipe screen
		DrawVoxel(&model,vx,vy,vz,modelang,modelang,modelang);
		
		// print some debug info
		PrintText(font8_bmp,8,yres-16,"FPS:%6.1f", fps);
		PrintText(font8_bmp,8,8,"angle: %d", (long)(camera.ang*180.0/PI));
		PrintText(font8_bmp,8,16,"x: %d", (long)camera.x);
		PrintText(font8_bmp,8,24,"y: %d", (long)camera.y);
		PrintText(font8_bmp,8,32,"z: %d", (long)camera.z);
		
		SDL_Flip( screen );
		cycles++;
	}
	
	// deinit
	free(model.data);
	free(zbuffer);
	free(filename);
	SDL_FreeSurface(font8_bmp);
	SDL_Quit();
	return 0;
}