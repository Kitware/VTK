/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include <math.h>
#include <iostream.h>
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyMapper.h"
#include "GL/gl.h"
#include "GL/glu.h"

#define MAX_LIGHTS 8

XVisualInfo *vtkOpenGLRenderWindow::GetDesiredVisualInfo()
{
  int           index;
  static int	attributes[50];
  XVisualInfo   *v;
  int           ms;

  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }

  // try getting exactly what we want
  index = 0;
  ms = this->MultiSamples;
  attributes[index++] = GLX_RGBA;
  attributes[index++] = GLX_RED_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_GREEN_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_BLUE_SIZE;
  attributes[index++] = 1;
  attributes[index++] = GLX_DEPTH_SIZE;
  attributes[index++] = 1;
  if (this->DoubleBuffer)
    {
    attributes[index++] = GLX_DOUBLEBUFFER;
    }
  // also try for STEREO
  // attributes[index++] = GLX_STEREO;
  
  // not all OpenGL implementations support MultiSamples

#ifdef GLX_SAMPLE_BUFFERS_SGIS
  if (this->MultiSamples > 1 )
    {
    attributes[index++] = GLX_SAMPLE_BUFFERS_SGIS;
    attributes[index++] = 1;
    attributes[index++] = GLX_SAMPLES_SGIS;
    attributes[index++] = ms;
    }
#endif
  attributes[index++] = None;

  v = glXChooseVisual(this->DisplayId, DefaultScreen(this->DisplayId), attributes );

  // if that failed, ditch the multi samples and try again
  if ((!v) && (this->MultiSamples > 1))
    {
    while ((ms > 1)&&(!v))
      {
      ms--;
      index = 9;
      if ( this->DoubleBuffer)
	{
	attributes[index++] = GLX_DOUBLEBUFFER;
	}
#ifdef GLX_SAMPLE_BUFFERS_SGIS
      attributes[index++] = GLX_SAMPLE_BUFFERS_SGIS;
      attributes[index++] = 1;
      attributes[index++] = GLX_SAMPLES_SGIS;
      attributes[index++] = ms;
#endif
      attributes[index++] = None;
    
      v = glXChooseVisual(this->DisplayId, DefaultScreen(this->DisplayId), 
			  attributes);
      }
    if (v)
      {
      vtkDebugMacro(<< "managed to get " << ms << " multisamples\n");
      }
    else
      {
      index = 9;
      if ( this->DoubleBuffer)
	{
	attributes[index++] = GLX_DOUBLEBUFFER;
	}
      attributes[index++] = None;
    
      v = glXChooseVisual(this->DisplayId, DefaultScreen(this->DisplayId), 
			  attributes);
      vtkDebugMacro(<< "unable to get any multisamples\n");
      }
    }

  // is we still don't have a visual lets ditch the double buffering
  if ((!v) && (this->DoubleBuffer))
    {
    index = 9;
    attributes[index++] = None;
    
    v = glXChooseVisual(this->DisplayId, DefaultScreen(this->DisplayId), 
			attributes);
    }

  // if we still don't have a visual then screw them
  if (!v) 
    {
    vtkErrorMacro(<< "Could not find a decent visual\n");
    }
  return ( v );
}

vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->ContextId = NULL;
  this->MultiSamples = 8;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;

  if ( this->WindowName ) 
    delete [] this->WindowName;
  this->WindowName = strdup("Visualization Toolkit - OpenGL");
}

// Description:
// free up memory & close the window
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  short cur_light;

  /* first delete all the old lights */
  // make sure we have been initialized 
  if (this->ContextId)
    {
    for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+MAX_LIGHTS; cur_light++)
      {
      glDisable((GLenum)cur_light);
      }
    glXDestroyContext( this->DisplayId, this->ContextId);
  
    // then close the old window 
    if (this->OwnWindow && this->DisplayId && this->WindowId)
      {
      XDestroyWindow(this->DisplayId,this->WindowId);
      }
    XSync(this->DisplayId,0);
    }
}

// Description:
// Begin the rendering process.
void vtkOpenGLRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);
}

// Description:
// End the rendering process and display the image.
void vtkOpenGLRenderWindow::Frame(void)
{
  glFlush();
  if (!this->AbortRender && this->DoubleBuffer&&this->SwapBuffers)
    {
    glXSwapBuffers(this->DisplayId, this->WindowId);
    vtkDebugMacro(<< " glXSwapBuffers\n");
    }
}
 

// Description:
// Update system if needed due to stereo rendering.
void vtkOpenGLRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
#ifdef HAVE_SETMON
	glFlush();
	system("/usr/gfx/setmon -n STR_RECT");
	glFlush();
	// make sure we are in full screen
	this->FullScreenOn();
        this->StereoStatus = 1;
#endif
	}
	break;
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 1;
	}
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
#ifdef HAVE_SETMON
	// restore the monitor 
	glFlush();
	system("/usr/gfx/setmon -n 72HZ");
	glFlush();
	// make sure we are out of full screen
	this->FullScreenOff();
#endif
        this->StereoStatus = 0;
	}
	break;
      case VTK_STEREO_RED_BLUE:
	{
        this->StereoStatus = 0;
	}
      }
    }
}

// Description:
// Specify various window parameters.
void vtkOpenGLRenderWindow::WindowConfigure()
{
  // this is all handles by the desiredVisualInfo method
}


// Description:
// Initialize the window for rendering.
void vtkOpenGLRenderWindow::WindowInitialize (void)
{
  XVisualInfo  *v, matcher;
  XSetWindowAttributes	attr;
  int x, y, width, height, nItems;
  XWindowAttributes winattr;
  XSizeHints xsh;

  xsh.flags = USSize;
  if ((this->Position[0] >= 0)&&(this->Position[1] >= 0))
    {
    xsh.flags |= USPosition;
    xsh.x =  (int)(this->Position[0]);
    xsh.y =  (int)(this->Position[1]);
    }
  
  x = ((this->Position[0] >= 0) ? this->Position[0] : 5);
  y = ((this->Position[1] >= 0) ? this->Position[1] : 5);
  width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  height = ((this->Size[1] > 0) ? this->Size[1] : 300);

  xsh.width  = width;
  xsh.height = height;

  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    }

  v = this->GetDesiredVisualInfo();
  attr.override_redirect = False;
  if (this->Borders == 0.0)
    attr.override_redirect = True;
  
    
  // create our own window ? 
  this->OwnWindow = 0;
  if (!this->WindowId)
    {
    this->ColorMap = XCreateColormap(this->DisplayId,
				     RootWindow( this->DisplayId, v->screen),
				     v->visual, AllocNone );

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = this->ColorMap;
    attr.event_mask = StructureNotifyMask | ExposureMask;
    
    // get a default parent if one has not been set.
    if (! this->ParentId)
      {
      this->ParentId = RootWindow(this->DisplayId, v->screen);
      }
    
    this->WindowId = 
      XCreateWindow(this->DisplayId,
		    this->ParentId,
		    x, y, width, height, 0, v->depth, InputOutput, v->visual,
		    CWBackPixel | CWBorderPixel | CWColormap | 
		    CWOverrideRedirect | CWEventMask, 
		    &attr);
    XStoreName(this->DisplayId, this->WindowId, this->WindowName);
    XSetNormalHints(this->DisplayId,this->WindowId,&xsh);
    this->OwnWindow = 1;
    }
  else
    {
    XChangeWindowAttributes(this->DisplayId,this->WindowId,
			    CWOverrideRedirect, &attr);
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    matcher.visualid = XVisualIDFromVisual(winattr.visual);
    XFree(v);
    v = XGetVisualInfo(this->DisplayId, VisualIDMask,
		       &matcher, &nItems);
    }

  // RESIZE THE WINDOW TO THE DESIRED SIZE
  vtkDebugMacro(<< "Resizing the xwindow\n");
  XResizeWindow(this->DisplayId,this->WindowId,
		((this->Size[0] > 0) ? 
		 (int)(this->Size[0]) : 256),
		((this->Size[1] > 0) ? 
		 (int)(this->Size[1]) : 256));
  XSync(this->DisplayId,False);

  this->ContextId = glXCreateContext(this->DisplayId, v, 0, GL_TRUE);
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  vtkDebugMacro(" Mapping the xwindow\n");
  XMapWindow(this->DisplayId, this->WindowId);
  XSync(this->DisplayId,False);
  XGetWindowAttributes(this->DisplayId,
		       this->WindowId,&winattr);
  while (winattr.map_state == IsUnmapped)
    {
    XGetWindowAttributes(this->DisplayId,
			 this->WindowId,&winattr);
    };
  
  vtkDebugMacro(<< " glMatrixMode ModelView\n");
  glMatrixMode( GL_MODELVIEW );

  vtkDebugMacro(<< " zbuffer enabled\n");
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );

  vtkDebugMacro(" texture stuff\n");
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

  // initialize blending for transparency
  vtkDebugMacro(<< " blend func stuff\n");
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  glEnable( GL_NORMALIZE );
  glAlphaFunc(GL_GREATER,0);
  
  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vtkOpenGLRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->ContextId)
    return;

  // now initialize the window 
  this->WindowInitialize();
}

// Description:
// Change the window to fill the entire screen.
void vtkOpenGLRenderWindow::SetFullScreen(int arg)
{
  int *temp;
  
  if (this->FullScreen == arg) return;
  
  if (!this->Mapped)
    {
    this->PrefFullScreen();
    return;
    }

  // set the mode 
  this->FullScreen = arg;
  if (this->FullScreen <= 0)
    {
    this->Position[0] = this->OldScreen[0];
    this->Position[1] = this->OldScreen[1];
    this->Size[0] = this->OldScreen[2]; 
    this->Size[1] = this->OldScreen[3];
    this->Borders = this->OldScreen[4];
    }
  else
    {
    // if window already up get its values 
    if (this->WindowId)
      {
      XWindowAttributes attribs;
      
      //  Find the current window size 
      XGetWindowAttributes(this->DisplayId, 
			   this->WindowId, &attribs);
      
      this->OldScreen[2] = attribs.width;
      this->OldScreen[3] = attribs.height;;

      temp = this->GetPosition();      
      this->OldScreen[0] = temp[0];
      this->OldScreen[1] = temp[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
      }
    }
  
  // remap the window 
  this->WindowRemap();

  // if full screen then grab the keyboard 
  if (this->FullScreen)
    {
    XGrabKeyboard(this->DisplayId,this->WindowId,
		  False,GrabModeAsync,GrabModeAsync,CurrentTime);
    }
  this->Modified();
}

// Description:
// Set the preferred window size to full screen.
void vtkOpenGLRenderWindow::PrefFullScreen()
{
  int *size;

  size = this->GetScreenSize();

  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  // don't show borders 
  this->Borders = 0;
}

// Description:
// Resize the window.
void vtkOpenGLRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+MAX_LIGHTS; cur_light++)
    {
    glDisable((GLenum)cur_light);
    }
  
  glXDestroyContext( this->DisplayId, this->ContextId);
  // then close the old window 
  if (this->OwnWindow)
    {
    XDestroyWindow(this->DisplayId,this->WindowId);
    }
  
  // set the default windowid 
  this->WindowId = this->NextWindowId;
  this->NextWindowId = (Window)NULL;

  // configure the window 
  this->WindowInitialize();
}


/*
void vtkOpenGLRenderWindow::SetPosition(int x,int y)
{
  // if we arent mappen then just set the ivars 
  if (!this->Mapped)
    {
    if ((this->Position[0] != x)||(this->Position[1] != y))
      {
      this->Modified();
      }
    this->Position[0] = x;
    this->Position[1] = y;
    return;
    }

  XMoveResizeWindow(this->DisplayId,this->WindowId,x,y, 
                    this->Size[0], this->Size[1]);
  XSync(this->DisplayId,False);
}
*/


// Description:
// Specify the size of the rendering window.
void vtkOpenGLRenderWindow::SetSize(int x,int y)
{
  // if we arent mappen then just set the ivars 
  if (!this->Mapped)
    {
    if ((this->Size[0] != x)||(this->Size[1] != y))
      {
      this->Modified();
      }
    this->Size[0] = x;
    this->Size[1] = y;
    return;
    }

  XResizeWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}



int vtkOpenGLRenderWindow::GetDesiredDepth()
{
  XVisualInfo *v;

  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  return v->depth;  
}

// Description:
// Get a visual from the windowing system.
Visual *vtkOpenGLRenderWindow::GetDesiredVisual ()
{
  XVisualInfo *v;

  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  return v->visual;  
}


// Description:
// Get a colormap from the windowing system.
Colormap vtkOpenGLRenderWindow::GetDesiredColormap ()
{
  XVisualInfo *v;

  if (this->ColorMap) return this->ColorMap;
  
  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  this->ColorMap = XCreateColormap(this->DisplayId,
				   RootWindow( this->DisplayId, v->screen),
				   v->visual, AllocNone );
  
  return this->ColorMap;  
}

void vtkOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkXRenderWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}


unsigned char *vtkOpenGLRenderWindow::GetPixelData(int x1, int y1, int x2, int y2, 
						 int front)
{
  long     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned char   *buffer;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;

  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  buffer = new unsigned char [4*(abs(x2 - x1)+1)];
  data = new unsigned char[(abs(x2 - x1) + 1)*(abs(y2 - y1) + 1)*3];

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    // read in a row of pixels 
    glReadPixels(x_low,yloop,(x_hi-x_low+1),1, 
		 GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      *p_data = buffer[xloop*4]; p_data++;
      *p_data = buffer[xloop*4+1]; p_data++;
      *p_data = buffer[xloop*4+2]; p_data++;
      }
    }
  
  delete [] buffer;

  return data;
}

void vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				       unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned char   *buffer;
  unsigned char   *p_data = NULL;

  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }

  buffer = new unsigned char [4*(abs(x2 - x1)+1)];

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  // now write the binary info one row at a time 
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      buffer[xloop*4] = *p_data; p_data++; 
      buffer[xloop*4+1] = *p_data; p_data++;
      buffer[xloop*4+2] = *p_data; p_data++;
      buffer[xloop*4+3] = 0xff;
      }
    /* write out a row of pixels */
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2f( 2.0 * (GLfloat)(x_low) / this->Size[0] - 1, 
                   2.0 * (GLfloat)(yloop) / this->Size[1] - 1);
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glDrawPixels((x_hi-x_low+1),1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
  
  delete [] buffer;
}

float *vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;
  float   *data = NULL;

  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  data = new float[ (width*height*4) ];

  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_FLOAT, data);

  return data;
}

void vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
				       float *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  if (front)
    {
    glDrawBuffer(GL_FRONT);
    }
  else
    {
    glDrawBuffer(GL_BACK);
    }

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }
  
  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }
  
  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  /* write out a row of pixels */
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2f( 2.0 * (GLfloat)(x_low) / this->Size[0] - 1, 
                 2.0 * (GLfloat)(y_low) / this->Size[1] - 1);
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
}

float *vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2  )
{
  int             y_low, y_hi;
  int             x_low, x_hi;
  int             width, height;
  float           *z_data = NULL;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  z_data = new float[width*height];

  glReadPixels( x_low, y_low, 
		width, height,
		GL_DEPTH_COMPONENT, GL_FLOAT,
		z_data );

  return z_data;
}

void vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
					  float *buffer )
{
  int             y_low, y_hi;
  int             x_low, x_hi;
  int             width, height;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    y_hi  = y2;
    }
  else
    {
    y_low = y2; 
    y_hi  = y1;
    }

  if (x1 < x2)
    {
    x_low = x1; 
    x_hi  = x2;
    }
  else
    {
    x_low = x2; 
    x_hi  = x1;
    }

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2f( 2.0 * (GLfloat)(x_low) / this->Size[0] - 1, 
                 2.0 * (GLfloat)(y_low) / this->Size[1] - 1);
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glDrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

}

void vtkOpenGLRenderWindow::MakeCurrent()
{
  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);
}
