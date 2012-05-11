#include <stdlib.h>
#include <stdio.h>

#include "FTGL.h"

#ifdef __APPLE_CC__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

#include "tb.h"

#ifndef FTGL_DO_NOT_USE_VECTORISER
#include "FTGLExtrdFont.h"
#include "FTGLOutlineFont.h"
#include "FTGLPolygonFont.h"
#endif

#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
#include "FTGLTextureFont.h"
#endif

#include "FTGLPixmapFont.h"
#include "FTGLBitmapFont.h"

// YOU'LL PROBABLY WANT TO CHANGE THESE

#ifdef __linux__
const char* FONT_FILE = "/usr/share/fonts/truetype/arial.ttf";
const char* FONT_INFO = "/usr/share/fonts/truetype/arial.ttf";
#else
#ifdef __APPLE_CC__
const char* FONT_FILE = "/Users/henry/Development/PROJECTS/FTGL/ftglcvs/FTGL/demo/arial.ttf";
const char* FONT_INFO = "/Users/henry/Development/PROJECTS/FTGL/ftglcvs/FTGL/demo/arial.ttf";
#else
#ifdef WIN32
const char* FONT_FILE = "C:\\WINNT\\Fonts\\arial.ttf";
const char* FONT_INFO = "C:\\WINNT\\Fonts\\arial.ttf";
#else
const char* FONT_FILE = "arial.ttf";
const char* FONT_INFO = "arial.ttf";
#endif
#endif
#endif

#define EDITING 1
#define INTERACTIVE 2

#define FTGL_BITMAP 0
#define FTGL_PIXMAP 1
#define FTGL_OUTLINE 2
#define FTGL_POLYGON 3
#define FTGL_EXTRUDE 4
#define FTGL_TEXTURE 5

#ifndef FTGL_DO_NOT_USE_VECTORISER
int current_font = FTGL_EXTRUDE;
#else
int current_font = FTGL_PIXMAP;
#endif

GLint w_win = 640, h_win = 480;
float posX, posY, posZ;
int mode = INTERACTIVE;
int carat = 0;

const char* fontfile;
const char* fontinfo;

//wchar_t myString[16] = { 0x6FB3, 0x9580};
wchar_t myString[16];

static FTFont* fonts[6];
static FTGLPixmapFont* infoFont;

void SetCamera(void);

void my_lighting()
{
   // Set up lighting.
   float light1_ambient[4]  = { 1.0, 1.0, 1.0, 1.0 };
   float light1_diffuse[4]  = { 1.0, 0.9, 0.9, 1.0 };
   float light1_specular[4] = { 1.0, 0.7, 0.7, 1.0 };
   float light1_position[4] = { -1.0, 1.0, 1.0, 0.0 };
   glLightfv(GL_LIGHT1, GL_AMBIENT,  light1_ambient);
   glLightfv(GL_LIGHT1, GL_DIFFUSE,  light1_diffuse);
   glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
   glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
   glEnable(GL_LIGHT1);

   float light2_ambient[4]  = { 0.2, 0.2, 0.2, 1.0 };
   float light2_diffuse[4]  = { 0.9, 0.9, 0.9, 1.0 };
   float light2_specular[4] = { 0.7, 0.7, 0.7, 1.0 };
   float light2_position[4] = { 1.0, -1.0, -1.0, 0.0 };
   glLightfv(GL_LIGHT2, GL_AMBIENT,  light2_ambient);
   glLightfv(GL_LIGHT2, GL_DIFFUSE,  light2_diffuse);
   glLightfv(GL_LIGHT2, GL_SPECULAR, light2_specular);
   glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
//   glEnable(GL_LIGHT2);

   float front_emission[4] = { 0.3, 0.2, 0.1, 0.0 };
   float front_ambient[4]  = { 0.2, 0.2, 0.2, 0.0 };
   float front_diffuse[4]  = { 0.95, 0.95, 0.8, 0.0 };
   float front_specular[4] = { 0.6, 0.6, 0.6, 0.0 };
   glMaterialfv(GL_FRONT, GL_EMISSION, front_emission);
   glMaterialfv(GL_FRONT, GL_AMBIENT, front_ambient);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, front_diffuse);
   glMaterialfv(GL_FRONT, GL_SPECULAR, front_specular);
   glMaterialf(GL_FRONT, GL_SHININESS, 16.0);
   glColor4fv(front_diffuse);

   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
   glEnable(GL_CULL_FACE);
   glColorMaterial(GL_FRONT, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   glEnable(GL_LIGHTING);
   glShadeModel(GL_SMOOTH);
}


void do_display ()
{
  switch( current_font)
  {
    case FTGL_BITMAP:
//      glDisable( GL_BLEND);
      break;
    case FTGL_PIXMAP:
//      glDisable( GL_TEXTURE_2D);
//      glEnable(GL_BLEND);
//      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
      break;
#ifndef FTGL_DO_NOT_USE_VECTORISER
    case FTGL_OUTLINE:
//      glDisable( GL_TEXTURE_2D);
//      glEnable( GL_LINE_SMOOTH);
//      glEnable(GL_BLEND);
//      glBlendFunc( GL_SRC_ALPHA, GL_ONE); // GL_ONE_MINUS_SRC_ALPHA
      break;
    case FTGL_POLYGON:
      glDisable( GL_BLEND);
      my_lighting();
      break;
    case FTGL_EXTRUDE:
      glEnable( GL_DEPTH_TEST);
      glDisable( GL_BLEND);
      my_lighting();
      break;
#endif
#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
    case FTGL_TEXTURE:
      glEnable( GL_TEXTURE_2D);
      glDisable( GL_DEPTH_TEST);
      my_lighting();
      glNormal3f( 0.0, 0.0, 1.0);
//      glDisable( GL_BLEND);
      break;
#endif
  }

  glColor3f( 1.0, 1.0, 1.0);
// If you do want to switch the color of bitmaps rendered with glBitmap,
// you will need to explicitly call glRasterPos3f (or its ilk) to lock
// in a changed current color.

  fonts[current_font]->render( myString);

  float x1, y1, z1, x2, y2, z2;
  fonts[current_font]->BBox( myString, x1, y1, z1, x2, y2, z2);
  
  // Draw the bounding box
  glDisable( GL_LIGHTING);
  glDisable( GL_TEXTURE_2D);
      glEnable( GL_LINE_SMOOTH);
      glEnable(GL_BLEND);
      glBlendFunc( GL_SRC_ALPHA, GL_ONE); // GL_ONE_MINUS_SRC_ALPHA

  glColor3f( 0.0, 1.0, 0.0);
  // Draw the front face
  glBegin( GL_LINE_LOOP);
    glVertex3f( x1, y1, z1);
    glVertex3f( x1, y2, z1);
    glVertex3f( x2, y2, z1);
    glVertex3f( x2, y1, z1);
  glEnd();
#ifndef FTGL_DO_NOT_USE_VECTORISER
  // Draw the back face
  if( current_font == FTGL_EXTRUDE && z1 != z2)
  {
    glBegin( GL_LINE_LOOP);
      glVertex3f( x1, y1, z2);
      glVertex3f( x1, y2, z2);
      glVertex3f( x2, y2, z2);
      glVertex3f( x2, y1, z2);
    glEnd();
  // Join the faces
    glBegin( GL_LINES);
      glVertex3f( x1, y1, z1);
      glVertex3f( x1, y1, z2);
      
      glVertex3f( x1, y2, z1);
      glVertex3f( x1, y2, z2);
      
      glVertex3f( x2, y2, z1);
      glVertex3f( x2, y2, z2);
      
      glVertex3f( x2, y1, z1);
      glVertex3f( x2, y1, z2);
    glEnd();
  }
#endif
    
    // Draw the baseline, Ascender and Descender
  glBegin( GL_LINES);
    glColor3f( 0.0, 0.0, 1.0);
    glVertex3f( 0.0, 0.0, 0.0);
    glVertex3f( fonts[current_font]->Advance( myString), 0.0, 0.0);
    
    glVertex3f( 0.0, fonts[current_font]->Ascender(), 0.0);
    glVertex3f( 0.0, fonts[current_font]->Descender(), 0.0);
    
  glEnd();
  
  // Draw the origin
  glColor3f( 1.0, 0.0, 0.0);
  glPointSize( 5.0);
  glBegin( GL_POINTS);
    glVertex3f( 0.0, 0.0, 0.0);
  glEnd();

  // draw the info
  int save_font = current_font;
  current_font = FTGL_PIXMAP;
  SetCamera();

  // draw mode
  glColor3f( 1.0, 1.0, 1.0);
  glRasterPos2i( 20 , h_win - ( 20 + infoFont->Ascender()));

  switch( mode)
  {
    case EDITING:
      infoFont->render("Edit Mode");
      break;
    case INTERACTIVE:
      break;
  }
  
  // draw font type
  glRasterPos2i( 20 , 20);
  switch( save_font)
  {
    case FTGL_BITMAP:
      infoFont->render("Bitmap Font");
      break;
    case FTGL_PIXMAP:
      infoFont->render("Pixmap Font");
      break;
#ifndef FTGL_DO_NOT_USE_VECTORISER
    case FTGL_OUTLINE:
      infoFont->render("Outline Font");
      break;
    case FTGL_POLYGON:
      infoFont->render("Polygon Font");
      break;
    case FTGL_EXTRUDE:
      infoFont->render("Extruded Font");
      break;
#endif
#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
    case FTGL_TEXTURE:
      infoFont->render("Texture Font");
      break;
#endif
  }
  
  glRasterPos2i( 20 , 20 + infoFont->Ascender() - infoFont->Descender());
  infoFont->render(fontfile);
  
  current_font = save_font;
  
  glutSwapBuffers();
}

void myinit ()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor( 0.13, 0.17, 0.32, 0.0);
  glColor3f( 1.0, 1.0, 1.0);
  
  glEnable( GL_CULL_FACE);
  glFrontFace( GL_CCW);
  
  glEnable( GL_DEPTH_TEST);
  
  glEnable( GL_POLYGON_OFFSET_LINE);
  glPolygonOffset( 1.0, 1.0); // ????
     
  SetCamera();

  fonts[FTGL_BITMAP] = new FTGLBitmapFont;
  fonts[FTGL_PIXMAP] = new FTGLPixmapFont;

#ifndef FTGL_DO_NOT_USE_VECTORISER
  fonts[FTGL_OUTLINE] = new FTGLOutlineFont;
  fonts[FTGL_POLYGON] = new FTGLPolygonFont;
  fonts[FTGL_EXTRUDE] = new FTGLExtrdFont;
#else
  fonts[FTGL_OUTLINE] = 
  fonts[FTGL_POLYGON] = 
  fonts[FTGL_EXTRUDE] = 0;
#endif

#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
  fonts[FTGL_TEXTURE] = new FTGLTextureFont;
#else
  fonts[FTGL_TEXTURE] = 0;
#endif

  for( int x = 0; x < 6; ++x)
  {
    if(!fonts[x])
      {
      continue;
      }

    if( !fonts[x]->Open( fontfile, false))
    {
      fprintf( stderr, "Failed to open font %s", fontfile);
      exit(1);
    }
    
    if( !fonts[x]->FaceSize( 144))
    {
      fprintf( stderr, "Failed to set size");
      exit(1);
    }
  
    fonts[x]->Depth(20);
    
    fonts[x]->CharMap(ft_encoding_unicode);
  }
  
  infoFont = new FTGLPixmapFont;
  
  if( !infoFont->Open( fontinfo, false))
  {
    fprintf( stderr, "Failed to open font %s", fontinfo);
    exit(1);
  }
  
  infoFont->FaceSize( 18);

  myString[0] = 65;
  myString[1] = 0;
  

  tbInit(GLUT_LEFT_BUTTON);
  tbAnimate( GL_FALSE);

}

#ifndef GLUTCALLBACK
#define GLUTCALLBACK
#endif

extern "C" {

void GLUTCALLBACK display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

     SetCamera();
  
  glPushMatrix();

  switch( current_font)
  {
    case FTGL_BITMAP:
    case FTGL_PIXMAP:
      glRasterPos2i( w_win / 2, h_win / 2);
      glTranslatef(  w_win / 2, h_win / 2, 0.0);
      break;
#ifndef FTGL_DO_NOT_USE_VECTORISER
    case FTGL_OUTLINE:
    case FTGL_POLYGON:
    case FTGL_EXTRUDE:
#endif
#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
    case FTGL_TEXTURE:
#endif
#if !defined(FTGL_DO_NOT_USE_VECTORISER) || !defined(FTGL_DO_NOT_USE_TEXTURE_FONT)
      tbMatrix();
      break;
#endif
  }
  
  do_display();

  glPopMatrix();

}

void GLUTCALLBACK parsekey(unsigned char key, int, int)
{
  switch (key)
  {
    case 27: exit(0); break;
    case 13:
      if( mode == EDITING)
      {
        mode = INTERACTIVE;
      }
      else
      {
        mode = EDITING;
        carat = 0;
      }
      break;
    case ' ':
      do
        {
        current_font++;
        if(current_font > 5)
          current_font = 0;
        } while (!fonts[current_font]);
      break;
    default:
      if( mode == INTERACTIVE)
      {
        myString[0] = key;
        myString[1] = 0;
        break;
      }
      else
      {
        myString[carat] = key;
        myString[carat + 1] = 0;
        carat = carat > 14 ? 15 : ++carat;
      }
  }
  
  glutPostRedisplay();

}


void GLUTCALLBACK parsekey_special(int key, int, int)
{
  switch (key)
  {
    case GLUT_KEY_UP:
      posY += 10;
      break;
    case GLUT_KEY_DOWN:
      posY -= 10;
      break;
    case GLUT_KEY_RIGHT:
      posX += 10;
      break;
    case GLUT_KEY_LEFT:
      posX -= 10;
      break;
  }
}

void GLUTCALLBACK motion(int x, int y)
{
  tbMotion( x, y);
}

void GLUTCALLBACK mouse(int button, int state, int x, int y)
{
  tbMouse( button, state, x, y);
}

void GLUTCALLBACK myReshape(int w, int h)
{
  glMatrixMode (GL_MODELVIEW);
  glViewport (0, 0, w, h);
  glLoadIdentity();
    
  w_win = w;
  h_win = h;
  SetCamera();
  
  tbReshape(w_win, h_win);
}

} // End of extern C

void SetCamera(void)
{
  switch( current_font)
  {
    case FTGL_BITMAP:
    case FTGL_PIXMAP:
      glMatrixMode( GL_PROJECTION);
      glLoadIdentity();
#ifndef FTGL_DO_NOT_USE_VECTORISER
      gluOrtho2D(0, w_win, 0, h_win);
#else
      glOrtho(0, w_win, 0, h_win, -1.0, 1.0);
#endif
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      break;
#ifndef FTGL_DO_NOT_USE_VECTORISER
    case FTGL_OUTLINE:
    case FTGL_POLYGON:
    case FTGL_EXTRUDE:
#endif
#ifndef FTGL_DO_NOT_USE_TEXTURE_FONT
    case FTGL_TEXTURE:
#endif
#if !defined(FTGL_DO_NOT_USE_VECTORISER) || !defined(FTGL_DO_NOT_USE_TEXTURE_FONT)
      glMatrixMode (GL_PROJECTION);
      glLoadIdentity ();
      gluPerspective( 90, (float)w_win / (float)h_win, 1, 1000);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluLookAt( 0.0, 0.0, (float)h_win / 2.0f, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
      break;
#endif
  }  
}


int main(int argc, char *argv[])
{
  fontfile = FONT_FILE;
  fontinfo = FONT_INFO;

  if (argc == 2)
      fontfile = fontinfo = argv[1];
    
  glutInit( &argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE | GLUT_MULTISAMPLE);
  glutInitWindowPosition(50, 50);
  glutInitWindowSize( w_win, h_win);
  glutCreateWindow("FTGL TEST");
  glutDisplayFunc(&display);
  glutKeyboardFunc(parsekey);
  glutMouseFunc(mouse);
    glutMotionFunc(motion);
  glutSpecialFunc(parsekey_special);
  glutReshapeFunc(myReshape);
  glutIdleFunc(display);

  myinit();

  glutMainLoop();

  return 0;
}
