// source changed by mrn@paus.ch/ max rheiner
// original source: henryj@paradise.net.nz

#include <stdio.h>
#include <stdlib.h> // exit()

#include "FTGL.h"

#ifdef __APPLE_CC__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

#ifndef FTGL_DO_NOT_USE_VECTORISER
#include "FTGLOutlineFont.h"
#include "FTGLPolygonFont.h"
#endif

#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
#include "FTGLTextureFont.h"
#endif

#include "FTGLBitmapFont.h"
#include "FTGLPixmapFont.h"

static FTFont* fonts[5];
static int width;
static int height;

static int point_size = 24;

#ifdef __linux__
const char* DEFAULT_FONT = "/usr/share/fonts/truetype/arial.ttf";
#else
#ifdef __APPLE_CC__
const char* DEFAULT_FONT = "/Users/henry/Development/PROJECTS/FTGL/ftglcvs/FTGL/demo/arial.ttf";
#else
#ifdef WIN32
const char* DEFAULT_FONT = "C:\\WINNT\\Fonts\\arial.ttf";
#else
const char* DEFAULT_FONT = "arial.ttf";
#endif
#endif
#endif

int file_exists( const char * filename );

void draw_scene();

void
my_init( const char* font_filename )
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#ifndef FTGL_DO_NOT_USE_VECTORISER
    fonts[0] = new FTGLOutlineFont;
    fonts[1] = new FTGLPolygonFont;
#else
    fonts[0] = 
    fonts[1] = 0;
#endif

#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
    fonts[2] = new FTGLTextureFont;
#else
    fonts[2] = 0;
#endif

    fonts[3] = new FTGLBitmapFont;
    fonts[4] = new FTGLPixmapFont;

    for (int i=0; i< 5; i++) {

       if(!fonts[i])
       {
         continue;
       }

        if (!fonts[i]->Open(font_filename)) {
			printf("Reading font %d from %s\n", i, font_filename);
			fprintf(stderr, "ERROR: Unable to open file %s\n", font_filename);
        }
        else {
			printf("Reading font %d from %s\n", i, font_filename);

            if (!fonts[i]->FaceSize(point_size)) {
            fprintf(stderr, "ERROR: Unable to set font face size %d\n", point_size);
            }

            // Try to load AFM font metrics
            const char* ext = strrchr(font_filename, '.');
            if (ext && !strcmp(ext, ".pfb"))
            {
            char *metrics = new char[strlen(font_filename)];
            strncpy(metrics, font_filename, ext - font_filename);
            strcpy(metrics + (ext - font_filename), ".afm");
            if (file_exists(metrics))
              {
              printf("Attaching font metrics from %s\n", metrics);;
              fonts[i]->Attach(metrics);
              }
            }
        }
    }
}

static void
do_ortho()
{
  int w;
  int h;
  GLdouble size;
  GLdouble aspect;

  w = width;
  h = height;
  aspect = (GLdouble)w / (GLdouble)h;

  // Use the whole window.
  glViewport(0, 0, w, h);

  // We are going to do some 2-D orthographic drawing.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  size = (GLdouble)((w >= h) ? w : h) / 2.0;
  if (w <= h) {
    aspect = (GLdouble)h/(GLdouble)w;
    glOrtho(-size, size, -size*aspect, size*aspect,
            -100000.0, 100000.0);
  }
  else {
    aspect = (GLdouble)w/(GLdouble)h;
    glOrtho(-size*aspect, size*aspect, -size, size,
            -100000.0, 100000.0);
  }

  // Make the world and window coordinates coincide so that 1.0 in
  // model space equals one pixel in window space.
  glScaled(aspect, aspect, 1.0);

   // Now determine where to draw things.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

#ifndef GLUTCALLBACK
#define GLUTCALLBACK
#endif

extern "C" {

void
GLUTCALLBACK my_display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    draw_scene();

    glutSwapBuffers();
}

void
GLUTCALLBACK my_reshape(int w, int h)
{
  width = w;
  height = h;

  do_ortho( );
}

void
GLUTCALLBACK my_handle_key(unsigned char key, int, int)
{
   switch (key) {

	   //!!ELLERS
   case 'q':   // Esc or 'q' Quits the program.
   case 27:    
	   {
       for (int i=0; i<5; i++) {
           if (fonts[i]) {
               delete fonts[i];
               fonts[i] = 0;
           }
       }
      exit(1);
	   }
      break;

   default:
      break;
   }
}

} // End of extern C

void
draw_scene()
{
   /* Set up some strings with the characters to draw. */
   unsigned int count = 0;
   char string[8][256];
   int i;
   for (i=1; i < 32; i++) { /* Skip zero - it's the null terminator! */
      string[0][count] = i;
      count++;
   }
   string[0][count] = '\0';

   count = 0;
   for (i=32; i < 64; i++) {
      string[1][count] = i;
      count++;
   }
   string[1][count] = '\0';

   count = 0;
   for (i=64; i < 96; i++) {
      string[2][count] = i;
      count++;
   }
   string[2][count] = '\0';

   count = 0;
   for (i=96; i < 128; i++) {
      string[3][count] = i;
      count++;
   }
   string[3][count] = '\0';

   count = 0;
   for (i=128; i < 160; i++) {
      string[4][count] = i;
      count++;
   }
   string[4][count] = '\0';

   count = 0;
   for (i=160; i < 192; i++) {
      string[5][count] = i;
      count++;
   }
   string[5][count] = '\0';

   count = 0;
   for (i=192; i < 224; i++) {
      string[6][count] = i;
      count++;
   }
   string[6][count] = '\0';

   count = 0;
   for (i=224; i < 256; i++) {
      string[7][count] = i;
      count++;
   }
   string[7][count] = '\0';


   glColor3f(1.0, 1.0, 1.0);

   for (int font = 0; font < 5; font++) {

       if(!fonts[font])
       {
         continue;
       }

       GLfloat x = -250.0;
       GLfloat y;
       GLfloat yild = 20.0;
       for (int j=0; j<4; j++) {
           y = 275.0-font*120.0-j*yild;
           if (font >= 3) {
               glRasterPos2f(x, y);
               fonts[font]->render(string[j]);
           }
           else {
#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
               if (font == 2) {
                   glEnable(GL_TEXTURE_2D);
                   glEnable(GL_BLEND);
                   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
               }
#endif
               glPushMatrix(); {
                   glTranslatef(x, y, 0.0);
                   fonts[font]->render(string[j]);
               } glPopMatrix();
               if (font == 2) {
                   glDisable(GL_TEXTURE_2D);
                   glDisable(GL_BLEND);
               }
           }
       }
   }
}


int 
file_exists( const char * filename )
{
	FILE * fp = fopen( filename, "r" );

	if ( fp == NULL )
	{
		// That fopen failed does _not_ definitely mean the file isn't there 
		// but for now this is ok
		return 0;
	}
	fclose( fp );
	return 1;
}

void
usage( const char * program )
{
  fprintf(stderr, "Usage %s <filename> <point_size>\n", program);
}

int
main(int argc, char **argv)
{
	//!!ELLERS  -- cleaned up
	const char * filename;

	glutInitWindowSize(600, 600);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE);

	glutCreateWindow("FTGL demo");

	if ( argc >= 2 ) 
	{
		if ( !file_exists( argv[ 1 ] ))
		{
			usage( argv[ 0 ]);
            fprintf(stderr, "Couldn't open file '%s'\n", argv[1]);
			exit( -1 );
		}
		filename = argv[ 1 ];

        if ( argc >= 3 ) 
          {
          point_size = atoi(argv[2]);
          }
	}
	else 
	{
		// try a default font
		filename = DEFAULT_FONT;

		if ( !file_exists( filename ))
		{
			usage( argv[ 0 ]);
            fprintf(stderr, "Couldn't open default file '%s'\n", filename);
			exit( -1 );
		}
	}

	my_init( filename );

	glutDisplayFunc(my_display);
	glutReshapeFunc(my_reshape);
	glutKeyboardFunc(my_handle_key);

	glutMainLoop();
	return 0;
}

