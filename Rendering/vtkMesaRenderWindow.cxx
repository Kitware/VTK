/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaProperty.h"
#include "vtkMesaTexture.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkMesaActor.h"
#include "vtkMesaPolyDataMapper.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

#define VTK_MAX_LIGHTS 8

// a couple of routines for offscreen rendering
void vtkOSMesaDestroyWindow(void *Window) 
{
  free(Window);
}

void *vtkOSMesaCreateWindow(int width, int height) 
{
  return malloc(width*height*4);
}


vtkMesaRenderWindow *vtkMesaRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMesaRenderWindow");
  if(ret)
    {
    return (vtkMesaRenderWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMesaRenderWindow;
}


XVisualInfo *vtkMesaRenderWindowTryForVisual(Display *DisplayId,
					       int doublebuff, int stereo)
{
  int           index;
  static int	attributes[50];

  // setup the default stuff we ask for
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
  if (doublebuff)
    {
    attributes[index++] = GLX_DOUBLEBUFFER;
    }
  if (stereo)
    {
    // also try for STEREO
    attributes[index++] = GLX_STEREO;
    }
  attributes[index++] = None;

  return glXChooseVisual(DisplayId,DefaultScreen(DisplayId),attributes);
}

XVisualInfo *vtkMesaRenderWindow::GetDesiredVisualInfo()
{
  XVisualInfo   *v = NULL;
  int           stereo = 0;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    this->OwnDisplay = 1;
    }

  // try every possibility stoping when we find one that works
  for (stereo = this->StereoCapableWindow; !v && stereo >= 0; stereo--)
    {
    if (v) 
      {
      XFree(v);
      }
    v = vtkMesaRenderWindowTryForVisual(this->DisplayId,
					this->DoubleBuffer, stereo);
    if (v && this->StereoCapableWindow && !stereo)
      {
      // requested a stereo capable window but we could not get one
      this->StereoCapableWindow = 0;
      }
    }
  for (stereo = this->StereoCapableWindow; !v && stereo >= 0; stereo--)
    {
    if (v) 
      {
      XFree(v);
      }
    v = vtkMesaRenderWindowTryForVisual(this->DisplayId,
					!this->DoubleBuffer, stereo);
    if (v)
      {
      this->DoubleBuffer = !this->DoubleBuffer;
      }
    if (v && this->StereoCapableWindow && !stereo)
      {
      // requested a stereo capable window but we could not get one
      this->StereoCapableWindow = 0;
      }
    }
  if (!v) 
    {
    vtkErrorMacro(<< "Could not find a decent visual\n");
    }
  return ( v );
}

vtkMesaRenderWindow::vtkMesaRenderWindow()
{
  this->OffScreenContextId = NULL;
  this->OffScreenWindow = NULL;
  this->ContextId = NULL;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;
  this->OwnWindow = 0;
  if ( this->WindowName ) 
    {
    delete [] this->WindowName;
    }
  this->WindowName = new char[strlen("Visualization Toolkit - Mesa")+1];
  strcpy( this->WindowName, "Visualization Toolkit - Mesa" );
}

// free up memory & close the window
vtkMesaRenderWindow::~vtkMesaRenderWindow()
{
  GLuint id;
  short cur_light;
  vtkMesaRenderer *ren;
  
  // make sure we have been initialized 
  if (this->ContextId || this->OffScreenContextId)
    {
    this->MakeCurrent();

    /* first delete all the old lights */
    for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+VTK_MAX_LIGHTS; cur_light++)
      {
      glDisable((GLenum)cur_light);
      }

    /* now delete all textures */
    glDisable(GL_TEXTURE_2D);
    for (int i = 1; i < this->TextureResourceIds->GetNumberOfIds(); i++)
      {
      id = (GLuint) this->TextureResourceIds->GetId(i);
#ifdef GL_VERSION_1_1
      if (glIsTexture(id))
	{
	glDeleteTextures(1, &id);
	}
#else
      if (glIsList(id))
        {
        glDeleteLists(id,1);
        }
#endif
      }

    // tell each of the renderers that this render window/graphics context
    // is being removed (the RendererCollection is removed by vtkRenderWindow's
    // destructor)
    this->Renderers->InitTraversal();
    for ( ren = (vtkMesaRenderer *) this->Renderers->GetNextItemAsObject();
	  ren != NULL;
	  ren = (vtkMesaRenderer *) this->Renderers->GetNextItemAsObject() )
      {
      ren->SetRenderWindow(NULL);
      }

    glFinish();
    if (this->OffScreenRendering)
      {
#ifdef MESA
      OSMesaDestroyContext(this->OffScreenContextId);
#endif
      this->OffScreenContextId = NULL;
      vtkOSMesaDestroyWindow(this->OffScreenWindow);
      this->OffScreenWindow = NULL;
      }
    else
      {
      glXDestroyContext( this->DisplayId, this->ContextId);
      // then close the old window 
      if (this->OwnWindow && this->DisplayId && this->WindowId)
	{
	XDestroyWindow(this->DisplayId,this->WindowId);
	this->WindowId = (Window)NULL;
	}
      }
    }
  this->ContextId = NULL;
}

// Begin the rendering process.
void vtkMesaRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId && !this->OffScreenContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}

// End the rendering process and display the image.
void vtkMesaRenderWindow::Frame(void)
{
  this->MakeCurrent();
  glFlush();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
    glXSwapBuffers(this->DisplayId, this->WindowId);
    vtkDebugMacro(<< " glXSwapBuffers\n");
    }
}
 

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkMesaRenderWindow::SetStereoCapableWindow(int capable)
{
  if (!this->WindowId)
    {
    vtkRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

// Initialize the window for rendering.
void vtkMesaRenderWindow::WindowInitialize (void)
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

  if (!this->OffScreenRendering)
    {
    // get the default display connection 
    if (!this->DisplayId)
      {
      this->DisplayId = XOpenDisplay((char *)NULL); 
      if (this->DisplayId == NULL) 
	{
	vtkErrorMacro(<< "bad X server connection.\n");
	}
      this->OwnDisplay = 1;
      }

    attr.override_redirect = False;
    if (this->Borders == 0.0)
      {
      attr.override_redirect = True;
      }
    
    // create our own window ? 
    this->OwnWindow = 0;
    if (!this->WindowId)
      {
      v = this->GetDesiredVisualInfo();
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
      matcher.screen = DefaultScreen(DisplayId);
      v = XGetVisualInfo(this->DisplayId, VisualIDMask | VisualScreenMask,
			 &matcher, &nItems);
      }
    
    // RESIZE THE WINDOW TO THE DESIRED SIZE
    vtkDebugMacro(<< "Resizing the xwindow\n");
    XResizeWindow(this->DisplayId,this->WindowId,
		  ((this->Size[0] > 0) ? 
		   (int)(this->Size[0]) : 300),
		  ((this->Size[1] > 0) ? 
		   (int)(this->Size[1]) : 300));
    XSync(this->DisplayId,False);

    this->ContextId = glXCreateContext(this->DisplayId, v, 0, GL_TRUE);
    this->MakeCurrent();

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
    // free the visual info
    if (v)
      {
      XFree(v);
      }
    this->Mapped = 1;
    this->Size[0] = width;
    this->Size[1] = height;      
    }
  else
    {
    this->DoubleBuffer = 0;
    if (!this->OffScreenWindow)
      {
      this->OffScreenWindow = vtkOSMesaCreateWindow(width,height);
      this->Size[0] = width;
      this->Size[1] = height;      
      this->OwnWindow = 1;
      }    
#ifdef MESA
    this->OffScreenContextId = OSMesaCreateContext(GL_RGBA, NULL);
#endif
    this->MakeCurrent();
    this->Mapped = 0;
    }
  
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
  glEnable(GL_BLEND);
  
  if (this->PointSmoothing)
    {
    glEnable(GL_POINT_SMOOTH);
    }
  else
    {
    glDisable(GL_POINT_SMOOTH);
    }

  if (this->LineSmoothing)
    {
    glEnable(GL_LINE_SMOOTH);
    }
  else
    {
    glDisable(GL_LINE_SMOOTH);
    }

  if (this->PolygonSmoothing)
    {
    glEnable(GL_POLYGON_SMOOTH);
    }
  else
    {
    glDisable(GL_POLYGON_SMOOTH);
    }

  glEnable( GL_NORMALIZE );
  glAlphaFunc(GL_GREATER,0);
  

}

// Initialize the rendering window.
void vtkMesaRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->ContextId || this->OffScreenContextId)
    {
    return;
    }
  
  // now initialize the window 
  this->WindowInitialize();
}

// Change the window to fill the entire screen.
void vtkMesaRenderWindow::SetFullScreen(int arg)
{
  if (this->OffScreenRendering)
    {
    return;
    }
  
  int *temp;
  
  if (this->FullScreen == arg) 
    {
    return;
    }
  
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

// Set the preferred window size to full screen.
void vtkMesaRenderWindow::PrefFullScreen()
{
  // use full screen 
  this->Position[0] = 0;
  this->Position[1] = 0;
  
  if (this->OffScreenRendering)
    {
    this->Size[0] = 1280;
    this->Size[1] = 1024;
    }
  else
    {
    int *size;
    
    size = this->GetScreenSize();
    
    this->Size[0] = size[0];
    this->Size[1] = size[1];
    }
  
  // don't show borders 
  this->Borders = 0;
}

// Resize the window.
void vtkMesaRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+VTK_MAX_LIGHTS; cur_light++)
    {
    glDisable((GLenum)cur_light);
    }
  
  if (this->OffScreenRendering)
    {
#ifdef MESA
    OSMesaDestroyContext(this->OffScreenContextId);
#endif
    this->OffScreenContextId = NULL;
    vtkOSMesaDestroyWindow(this->OffScreenWindow);
    this->OffScreenWindow = NULL;
    }
  else
    {
    glXDestroyContext( this->DisplayId, this->ContextId);
    // then close the old window 
    if (this->OwnWindow)
      {
      XDestroyWindow(this->DisplayId,this->WindowId);
      }
    }
  
  // set the default windowid 
  this->WindowId = this->NextWindowId;
  this->NextWindowId = (Window)NULL;

  // configure the window 
  this->WindowInitialize();
}


// Specify the size of the rendering window.
void vtkMesaRenderWindow::SetSize(int x,int y)
{
  if ((this->Size[0] == x)&&(this->Size[1] == y))
    {
    return;
    }
  
  this->Modified();
  this->Size[0] = x;
  this->Size[1] = y;
  
  
  if (this->OffScreenRendering && this->OffScreenWindow)
    {
    vtkRenderer *ren;
    // Disconnect renderers from this render window.
    vtkRendererCollection *renderers = this->Renderers;
    renderers->Register(this);
    this->Renderers->Delete();
    this->Renderers = vtkRendererCollection::New();
    renderers->InitTraversal();
    while ( (ren = renderers->GetNextItem()) )
      {
      ren->SetRenderWindow(NULL);
      }
    
#ifdef MESA
    OSMesaDestroyContext(this->OffScreenContextId);
#endif
    this->OffScreenContextId = NULL;
    vtkOSMesaDestroyWindow(this->OffScreenWindow);
    this->OffScreenWindow = NULL;      
    this->WindowInitialize();
    
    // Add the renders back into the render window.
    renderers->InitTraversal();
    while ( (ren = renderers->GetNextItem()) )
      {
      this->AddRenderer(ren);
      }
    renderers->Delete();
    }
  else
    {
    if (this->Mapped)
      {
      XResizeWindow(this->DisplayId,this->WindowId,x,y);
      XSync(this->DisplayId,False);
      }
    }
}



int vtkMesaRenderWindow::GetDesiredDepth()
{
  XVisualInfo *v;
  int depth = 0;
  
  // get the default visual to use 
  v = this->GetDesiredVisualInfo();
  
  if (v)
    {
    depth = v->depth;  
    XFree(v);
    }

  return depth;
}

// Get a visual from the windowing system.
Visual *vtkMesaRenderWindow::GetDesiredVisual ()
{
  XVisualInfo *v;
  Visual *vis = NULL;
  
  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  if (v)
    {
    vis = v->visual;  
    XFree(v);
    }
  
  return vis;  
}


// Get a colormap from the windowing system.
Colormap vtkMesaRenderWindow::GetDesiredColormap ()
{
  XVisualInfo *v;
  
  if (this->ColorMap) 
    {
    return this->ColorMap;
    }
  
  // get the default visual to use 
  v = this->GetDesiredVisualInfo();

  this->ColorMap = XCreateColormap(this->DisplayId,
				   RootWindow( this->DisplayId, v->screen),
				   v->visual, AllocNone ); 
  if (v)
    {
    XFree(v);
    }

  return this->ColorMap;  
}

void vtkMesaRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkXRenderWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "OffScreenContextId: " << this->OffScreenContextId << "\n";
}

int vtkMesaRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
    {
    size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
    }
  else
    {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
    }
}

unsigned char *vtkMesaRenderWindow::GetPixelData(int x1, int y1, 
						   int x2, int y2, 
						   int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  unsigned char   *data = NULL;

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

  if (front)
    {
    glReadBuffer(GL_FRONT);
    }
  else
    {
    glReadBuffer(GL_BACK);
    }

  data = new unsigned char[(x_hi - x_low + 1)*(y_hi - y_low + 1)*3];

#ifdef sparc
  // We need to read the image data one row at a time and convert it
  // from RGBA to RGB to get around a bug in Sun Mesa 1.1
  long    xloop, yloop;
  unsigned char *buffer;
  unsigned char *p_data = NULL;
  
  buffer = new unsigned char [4*(x_hi - x_low + 1)];
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    // read in a row of pixels
    glReadPixels(x_low,yloop,(x_hi-x_low+1),1,
		 GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (xloop = 0; xloop <= x_hi-x_low; xloop++)
      {
      *p_data = buffer[xloop*4]; p_data++;
      *p_data = buffer[xloop*4+1]; p_data++;
      *p_data = buffer[xloop*4+2]; p_data++;
      }
    }
  
  delete [] buffer;  
#else
  // If the Sun bug is ever fixed, then we could use the following
  // technique which provides a vast speed improvement on the SGI
  
  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels(x_low, y_low, x_hi-x_low+1, y_hi-y_low+1, GL_RGB,
               GL_UNSIGNED_BYTE, data);
#endif
  
  return data;
}

void vtkMesaRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
				       unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window 
  this->MakeCurrent();

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

#ifdef sparc
  // We need to read the image data one row at a time and convert it
  // from RGBA to RGB to get around a bug in Sun Mesa 1.1
  long    xloop, yloop;
  unsigned char *buffer;
  unsigned char *p_data = NULL;
  
  buffer = new unsigned char [4*(x_hi - x_low + 1)];

  // now write the binary info one row at a time
  glDisable(GL_BLEND);
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
    {
    for (xloop = 0; xloop <= x_hi - x_low; xloop++)
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
    glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1),
		   (2.0 * (GLfloat)(yloop) / this->Size[1] - 1),
		   -1.0 );
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glDrawPixels((x_hi-x_low+1),1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
  glEnable(GL_BLEND);
#else
  // If the Sun bug is ever fixed, then we could use the following
  // technique which provides a vast speed improvement on the SGI
  
  // now write the binary info
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1), 
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDisable(GL_BLEND);
  glDrawPixels((x_hi-x_low+1), (y_hi - y_low + 1),
               GL_RGB, GL_UNSIGNED_BYTE, data);
  glEnable(GL_BLEND);
#endif
}

float *vtkMesaRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;
  float   *data = NULL;

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

void vtkMesaRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
				       float *data, int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window 
  this->MakeCurrent();

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
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1), 
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
		 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  if (!blend)
    {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    glEnable(GL_BLEND);
    }
  else
    {
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    }
}


unsigned char *vtkMesaRenderWindow::GetRGBACharPixelData(int x1, int y1, int x2, 
							 int y2, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;
  unsigned char *data = NULL;


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


  data = new unsigned char[ (width*height)*4 ];


  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
		data);


  return data;
}


void vtkMesaRenderWindow::SetRGBACharPixelData(int x1, int y1, int x2, 
					       int y2, unsigned char *data, 
					       int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;


  // set the current window
  this->MakeCurrent();


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
  glRasterPos3f( (2.0 * (GLfloat)(x_low) / this->Size[0] - 1),
                 (2.0 * (GLfloat)(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();


  if (!blend)
    {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, 
		  data);
    glEnable(GL_BLEND);
    }
  else
    {
    glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, 
		  data);
    }
}





float *vtkMesaRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2  )
{
  int             y_low;
  int             x_low;
  int             width, height;
  float           *z_data = NULL;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    }
  else
    {
    y_low = y2; 
    }

  if (x1 < x2)
    {
    x_low = x1; 
    }
  else
    {
    x_low = x2; 
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

void vtkMesaRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
					  float *buffer )
{
  int             y_low;
  int             x_low;
  int             width, height;

  // set the current window 
  this->MakeCurrent();

  if (y1 < y2)
    {
    y_low = y1; 
    }
  else
    {
    y_low = y2; 
    }

  if (x1 < x2)
    {
    x_low = x1; 
    }
  else
    {
    x_low = x2; 
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
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glDrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

}

void vtkMesaRenderWindow::MakeCurrent()
{
  // set the current window 
  if (this->OffScreenRendering)
    {
    if (this->OffScreenContextId) 
      {
#ifdef MESA
      if (OSMesaMakeCurrent(this->OffScreenContextId, 
			    this->OffScreenWindow, GL_UNSIGNED_BYTE, 
			    this->Size[0], this->Size[1]) != GL_TRUE) 
	{
	vtkWarningMacro("failed call to OSMesaMakeCurrent");
	}
#endif
      }
    }
  else
    {
    if (this->ContextId && (this->ContextId != glXGetCurrentContext()))
      {
      glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);
      }
    }
}

void vtkMesaRenderWindow::SetOffScreenRendering(int i)
{
  if (this->OffScreenRendering == i)
    {
    return;
    }
  
#ifdef MESA
  // invoke super
  this->vtkRenderWindow::SetOffScreenRendering(i);
  
  // setup everything
  if (i)
    {
    this->ScreenDoubleBuffer = this->DoubleBuffer;    
    this->DoubleBuffer = 0;
    this->ScreenMapped = this->Mapped;
    this->Mapped = 0;
    if (!this->OffScreenWindow)
      {
      this->WindowInitialize();
      }    
    }
  else
    {
    if (this->OffScreenWindow)
      {
      OSMesaDestroyContext(this->OffScreenContextId);
      this->OffScreenContextId = NULL;
      vtkOSMesaDestroyWindow(this->OffScreenWindow);
      this->OffScreenWindow = NULL;      
      }
    this->DoubleBuffer = this->ScreenDoubleBuffer;
    this->Mapped = this->ScreenMapped;
    this->MakeCurrent();
    // reset the size based on the screen window
    this->GetSize();
    this->WindowInitialize();
    }
#endif
}

void *vtkMesaRenderWindow::GetGenericContext()
{
  if (this->OffScreenRendering)
    {
    return (void *)this->OffScreenContextId;
    }
  else
    {
    return (void *)this->ContextId;
    }
}

void *vtkMesaRenderWindow::GetGenericWindowId()
{
  if (this->OffScreenRendering)
    {
    return (void *)this->OffScreenWindow;
    }
  else
    {
    return (void *)this->WindowId;
    }
}
