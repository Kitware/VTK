/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrRenW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <iostream.h>
#include "OglrRenW.hh"
#include "OglrRen.hh"
#include "OglrProp.hh"
#include "OglrText.hh"
#include "OglrCam.hh"
#include "OglrLgt.hh"
#include "GL/gl.h"
#include "GL/glu.h"

#define MAX_LIGHTS 8

XVisualInfo *vtkOglrRenderWindow::GetDesiredVisualInfo()
{
  int           index;
  static int	attributes[50];
  XVisualInfo   *v, vinfo;
  int           value;
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
  if (this->DoubleBuffer)
    {
    attributes[index++] = GLX_DOUBLEBUFFER;
    }
  attributes[index++] = GLX_DEPTH_SIZE;
  attributes[index++] = 1;
  if (this->MultiSamples > 1 )
    {
    attributes[index++] = GLX_SAMPLE_BUFFER_SGIS;
    attributes[index++] = 1;
    attributes[index++] = GLX_SAMPLES_SGIS;
    attributes[index++] = ms;
    }
  attributes[index++] = None;

  v = glXChooseVisual(this->DisplayId, DefaultScreen(this->DisplayId), attributes );

  // if that failed, ditch the multi samples and try again
  if ((!v) && (this->MultiSamples > 1))
    {
    while ((ms > 1)&&(!v))
      {
      ms--;
      index = 0;
      attributes[index++] = GLX_RGBA;
      attributes[index++] = GLX_RED_SIZE;
      attributes[index++] = 1;
      attributes[index++] = GLX_GREEN_SIZE;
      attributes[index++] = 1;
      attributes[index++] = GLX_BLUE_SIZE;
      attributes[index++] = 1;
      if ( this->DoubleBuffer)
	{
	attributes[index++] = GLX_DOUBLEBUFFER;
	}
      attributes[index++] = GLX_DEPTH_SIZE;
      attributes[index++] = 1;
      attributes[index++] = GLX_SAMPLE_BUFFER_SGIS;
      attributes[index++] = 1;
      attributes[index++] = GLX_SAMPLES_SGIS;
      attributes[index++] = this->MultiSamples;
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
      index = 0;
      attributes[index++] = GLX_RGBA;
      attributes[index++] = GLX_RED_SIZE;
      attributes[index++] = 1;
      attributes[index++] = GLX_GREEN_SIZE;
      attributes[index++] = 1;
      attributes[index++] = GLX_BLUE_SIZE;
      attributes[index++] = 1;
      if ( this->DoubleBuffer)
	{
	attributes[index++] = GLX_DOUBLEBUFFER;
	}
      attributes[index++] = GLX_DEPTH_SIZE;
      attributes[index++] = 1;
      attributes[index++] = None;
    
      v = glXChooseVisual(this->DisplayId, DefaultScreen(this->DisplayId), 
			  attributes);
      vtkDebugMacro(<< "unable to get any multisamples\n");
      }
    }

  // is we still don't have a visual lets ditch the double buffering
  if ((!v) && (this->DoubleBuffer))
    {
    index = 0;
    attributes[index++] = GLX_RGBA;
    attributes[index++] = GLX_RED_SIZE;
    attributes[index++] = 1;
    attributes[index++] = GLX_GREEN_SIZE;
    attributes[index++] = 1;
    attributes[index++] = GLX_BLUE_SIZE;
    attributes[index++] = 1;
    attributes[index++] = GLX_DEPTH_SIZE;
    attributes[index++] = 1;
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

vtkOglrRenderWindow::vtkOglrRenderWindow()
{
  this->ContextId = NULL;
  this->MultiSamples = 8;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;
  this->StereoType = VTK_STEREO_CRYSTAL_EYES;

  strcpy(this->Name,"Visualization Toolkit - OpenGL");
}

// Description:
// Create a OpenGL specific light.
vtkLightDevice *vtkOglrRenderWindow::MakeLight()
{
  vtkOglrLight *light;

  light = new vtkOglrLight;
  return (vtkLightDevice *)light;
}

// Description:
// Create a OpenGL specific renderer.
vtkRenderer *vtkOglrRenderWindow::MakeRenderer()
{
  vtkOglrRenderer *ren;

  ren = new vtkOglrRenderer;
  this->AddRenderers(ren);

  // by default we are its parent
  ren->SetRenderWindow((vtkRenderWindow*)this);
  
  return (vtkRenderer *)ren;
}

// Description:
// Create a OpenGL specific camera.
vtkCameraDevice *vtkOglrRenderWindow::MakeCamera()
{
  vtkOglrCamera *camera;

  camera = new vtkOglrCamera;
  return (vtkCameraDevice *)camera;
}

// Description:
// Create a OpenGL specific property.
vtkPropertyDevice *vtkOglrRenderWindow::MakeProperty()
{
  vtkOglrProperty *property;

  property = new vtkOglrProperty;
  return (vtkPropertyDevice *)property;
}

// Description:
// Create a OpenGL specific texture.
vtkTextureDevice *vtkOglrRenderWindow::MakeTexture()
{
  vtkOglrTexture *texture;

  texture = new vtkOglrTexture;
  return (vtkTextureDevice *)texture;
}

// Description:
// Begin the rendering process.
void vtkOglrRenderWindow::Start(void)
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
void vtkOglrRenderWindow::Frame(void)
{
  glFlush();
  if (this->DoubleBuffer)
    {
    glXSwapBuffers(this->DisplayId, this->WindowId);
    vtkDebugMacro(<< " glXSwapBuffers\n");
    }
}
 

// Description:
// Update system if needed due to stereo rendering.
void vtkOglrRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	glFlush();
	system("/usr/gfx/setmon STR_TOP");
	glFlush();
	// make sure we are in full screen
        this->StereoStatus = 1;
	// this->FullScreenOn();
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
	// restore the monitor 
	glFlush();
	system("/usr/gfx/setmon 72HZ");
	glFlush();
	// make sure we are in full screen
        this->StereoStatus = 0;
	// this->FullScreenOff();
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
void vtkOglrRenderWindow::WindowConfigure()
{
  // this is all handles by the desiredVisualInfo method
}


// Description:
// Initialize the window for rendering.
void vtkOglrRenderWindow::WindowInitialize (void)
{
  XVisualInfo  *v;
  XSetWindowAttributes	attr;
  Window  wins[2];
  int x,y,width,height;
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

    attr.border_pixel = 0;
    attr.colormap = this->ColorMap;

    this->WindowId = 
      XCreateWindow(this->DisplayId,
		    RootWindow(this->DisplayId, v->screen), 
		    x, y, width, height, 0, v->depth, InputOutput, v->visual,
		    CWBorderPixel|CWColormap|CWOverrideRedirect, &attr);
    XStoreName(this->DisplayId, this->WindowId, this->Name);
    XSetNormalHints(this->DisplayId,this->WindowId,&xsh);
    this->OwnWindow = 1;
    }
  else
    {
    XChangeWindowAttributes(this->DisplayId,this->WindowId,
			    CWOverrideRedirect, &attr);
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
  
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  // just checking
  glGetIntergerv(GL_RED_BITS, &rs);
  cerr << "red size " << rs << "\n";

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
  glEnable( GL_BLEND );

  glEnable( GL_NORMALIZE );

  this->Mapped = 1;
}

// Description:
// Initialize the rendering window.
void vtkOglrRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->ContextId)
    return;

  // now initialize the window 
  this->WindowInitialize();
}

// Description:
// Change the window to fill the entire screen.
void vtkOglrRenderWindow::SetFullScreen(int arg)
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
void vtkOglrRenderWindow::PrefFullScreen()
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
void vtkOglrRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+MAX_LIGHTS; cur_light++)
    {
    glDisable(cur_light);
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

// Description:
// Specify the size of the rendering window.
void vtkOglrRenderWindow::SetSize(int x,int y)
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



int vtkOglrRenderWindow::GetDesiredDepth()
{
  XVisualInfo *v;

  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  return v->depth;  
}

// Description:
// Get a visual from the windowing system.
Visual *vtkOglrRenderWindow::GetDesiredVisual ()
{
  XVisualInfo *v;

  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  return v->visual;  
}


// Description:
// Get a colormap from the windowing system.
Colormap vtkOglrRenderWindow::GetDesiredColormap ()
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

void vtkOglrRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkXRenderWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}


unsigned char *vtkOglrRenderWindow::GetPixelData(int x1, int y1, int x2, int y2, 
						 int front)
{
  long     xloop,yloop;
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned long   *buffer;
  unsigned char   *data = NULL;
  unsigned char   *p_data = NULL;

  // set the current window 
  glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);

  buffer = new unsigned long[abs(x2 - x1)+1];
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
    glReadPixels(x_low,yloop,(x_hi-x_low+1),1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (xloop = 0; xloop <= (abs(x2-x1)); xloop++)
      {
      *p_data = buffer[xloop] & (0x000000ff); p_data++;
      *p_data = (buffer[xloop] & (0x0000ff00)) >> 8; p_data++;
      *p_data = (buffer[xloop] & (0x00ff0000)) >> 16; p_data++;
      }
    }
  
  delete [] buffer;

  return data;
}

void vtkOglrRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				       unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     xloop,yloop;
  unsigned long   *buffer;
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

  buffer = new unsigned long[4*(abs(x2 - x1)+1)];

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
      buffer[xloop] = 0xff000000 + *p_data; p_data++; 
      buffer[xloop] += (*p_data) << 8; p_data++;
      buffer[xloop] += (*p_data) << 16; p_data++;
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
