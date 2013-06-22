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
	camx += (keystatus[SDLK_UP]-keystatus[SDLK_DOWN])*cos(camang)*timesync*.125*speed;
	camy += (keystatus[SDLK_UP]-keystatus[SDLK_DOWN])*sin(camang)*timesync*.125*speed;
	camz += (keystatus[SDLK_PAGEDOWN]-keystatus[SDLK_PAGEUP])*timesync*.25*speed;
	camang += (keystatus[SDLK_RIGHT]-keystatus[SDLK_LEFT])*timesync*.002;
	while( camang > PI*2 )
		camang -= PI*2;
	while( camang < 0 )
		camang += PI*2;
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
	voxbit_t *bit = NULL;
	voxbit_t *nextbit = NULL;
	voxbit_t *firstbit = NULL;
	unsigned long d;
	double dx, dy, dz;
	double ax, ay;
	int cosang, sinang;
	double cosyaw, sinyaw, cospitch, sinpitch, cosroll, sinroll;
	long offX, offY, offZ;

	// drawing variables
	long hx, hy, hz;
	long voxX, voxY, voxZ;
	int x, y;
	int screenindex;
	int index;
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
	cosang = ((int)(cos(camang)*64))<<8;
	sinang = ((int)(sin(camang)*64))<<8;
	hx = xres>>1; hy = yres>>1; hz = hx;
	sprsize = 128.0*((double)xres/320.0);

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
				
				// first, calculate the distance between the player and the voxbit (performing rotation)
				dx = posx - camx + (offX*cospitch*cosyaw) + (offY*cosroll*sinyaw + offY*sinroll*sinpitch*cosyaw) + offZ*sinroll*sinyaw - offZ*cosroll*sinpitch*cosyaw;
				dy = camy - posy - (offX*cospitch*sinyaw) + (offY*cosroll*cosyaw - offY*sinroll*sinpitch*sinyaw) + offZ*sinroll*cosyaw + offZ*cosroll*sinpitch*sinyaw;
				dz = posz - camz + (offX*sinpitch) - (offY*sinroll*cospitch) + (offZ*cosroll*cospitch);
				d = sqrt(dx*dx + dy*dy);
				
				// get the onscreen direction to the voxbit
				ax = (dx*sinang)+(dy*cosang);
				ay = (dx*cosang)-(dy*sinang);
				if( ay <= 0 ) continue; // bit is outside the view frustrum
				
				// allocate memory for the voxbit
				bit = (voxbit_t *) malloc(sizeof(voxbit_t));
				bit->color = color;
				
				// compute and store final information about the voxbit
				bit->sx = (ax*(hx/ay)*-1)+hx; // onscreen position x
				d *= cos((bit->sx-hx)*(1.0/xres)*(PI/2.0)); // correct fishbowl effect
				if( d < CLIPNEAR || d > CLIPFAR ) {
					free(bit);
					continue;
				}
				bit->d = d;
				bit->sy = hy+((hz/(double)d)*dz); // onscreen position y
				
				bitsize = 1.0/d*sprsize;
				bit->x1 = max( bit->sx-bitsize-1, 0 );
				bit->x2 = min( bit->sx+bitsize+1, xres );
				bit->y1 = max( bit->sy-bitsize-1, 0 );
				bit->y2 = min( bit->sy+bitsize+1, yres );
				
				// place the voxbit in the list
				if( firstbit == NULL ) {
					firstbit = bit;
					bit->next = NULL;
				}
				else {
					bit->next = firstbit;
					firstbit = bit;
				}
			}
		}
	}
	
	// draw the voxel model
	if( firstbit != NULL ) {
		for( bit=firstbit;bit!=NULL;bit=bit->next ) {
			for( y=bit->y1; y<bit->y2; y++ ) {
				p = (Uint8 *)screen->pixels + y * screen->pitch; // calculate the row we are drawing in
				screenindex=bit->x1*screen->format->BytesPerPixel;
				for( x=bit->x1; x<bit->x2; x++ ) {
					if( bit->d < zbuffer[x+y*xres] || !zbuffer[x+y*xres] ) {
						*(Uint32 *)((Uint8 *)p + screenindex) = bit->color;
						zbuffer[x+y*xres]=bit->d;
					}
					screenindex+=screen->format->BytesPerPixel;
				}
			}
		}
		
		// free the bit list
		bit=firstbit;
		do {
			nextbit=bit->next;
			free(bit);
			bit=nextbit;
		} while( nextbit != NULL );
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
			vx = camx;
			vy = camy;
			vz = camz;
			modelang = camang;
		}
		
		// rendering
		memset( zbuffer, 0, xres*yres*sizeof(long) );
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,50,50,150)); // wipe screen
		DrawVoxel(&model,vx,vy,vz,modelang,modelang,modelang);
		
		// print some debug info
		PrintText(font8_bmp,8,yres-16,"FPS:%6.1f", fps);
		PrintText(font8_bmp,8,8,"angle: %d", (long)(camang*180.0/PI));
		PrintText(font8_bmp,8,16,"x: %d", (long)camx);
		PrintText(font8_bmp,8,24,"y: %d", (long)camy);
		PrintText(font8_bmp,8,32,"z: %d", (long)camz);
		
		SDL_Flip( screen );
		cycles++;
	}
	
	// deinit
	SDL_FreeSurface(screen);
	SDL_FreeSurface(font8_bmp);
	free(model.data);
	free(zbuffer);
	free(filename);
	SDL_Quit();
	return 0;
}