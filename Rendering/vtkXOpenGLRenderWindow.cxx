/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXOpenGLRenderWindow.cxx
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
#include "vtkXOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkIdList.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"



//-----------------------------------------------------------------------------
vtkXOpenGLRenderWindow* vtkXOpenGLRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXOpenGLRenderWindow");
  if(ret)
    {
    return (vtkXOpenGLRenderWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXOpenGLRenderWindow;
}




#define MAX_LIGHTS 8

XVisualInfo *vtkXOpenGLRenderWindowTryForVisual(Display *DisplayId,
					       int doublebuff, int stereo,
					       int multisamples)
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
  if (multisamples)
    {
#ifdef GLX_SAMPLE_BUFFERS_SGIS
    attributes[index++] = GLX_SAMPLE_BUFFERS_SGIS;
    attributes[index++] = 1;
    attributes[index++] = GLX_SAMPLES_SGIS;
    attributes[index++] = multisamples;
#endif
    }
    
  attributes[index++] = None;

  return glXChooseVisual(DisplayId, DefaultScreen(DisplayId), attributes );
}

XVisualInfo *vtkXOpenGLRenderWindow::GetDesiredVisualInfo()
{
  XVisualInfo   *v = NULL;
  int           multi;
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
    for (multi = this->MultiSamples; !v && multi >= 0; multi--)
      {
      if (v) 
	{
	XFree(v);
	}
      v = vtkXOpenGLRenderWindowTryForVisual(this->DisplayId,
					    this->DoubleBuffer, 
					    stereo, multi);
      if (v && this->StereoCapableWindow && !stereo)
	{
	// requested a stereo capable window but we could not get one
	this->StereoCapableWindow = 0;
	}
      }
    }
  for (stereo = this->StereoCapableWindow; !v && stereo >= 0; stereo--)
    {
    for (multi = this->MultiSamples; !v && multi >= 0; multi--)
      {
      if (v) 
	{
	XFree(v);
	}
      v = vtkXOpenGLRenderWindowTryForVisual(this->DisplayId,
					    !this->DoubleBuffer, 
					    stereo, multi);
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
    }
  if (!v) 
    {
    vtkErrorMacro(<< "Could not find a decent visual\n");
    }
  return ( v );
}

vtkXOpenGLRenderWindow::vtkXOpenGLRenderWindow()
{
  this->ContextId = NULL;
  this->MultiSamples = vtkXOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;
  this->OwnWindow = 0;
  if ( this->WindowName ) 
    delete [] this->WindowName;
  this->WindowName = new char[strlen("Visualization Toolkit - OpenGL")+1];
    strcpy( this->WindowName, "Visualization Toolkit - OpenGL" );
}

// free up memory & close the window
vtkXOpenGLRenderWindow::~vtkXOpenGLRenderWindow()
{
  GLuint id;
  short cur_light;
  vtkOpenGLRenderer *ren;
  
  // make sure we have been initialized 
  if (this->ContextId)
    {
    this->MakeCurrent();

    /* first delete all the old lights */
    for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+MAX_LIGHTS; cur_light++)
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
    for ( ren = (vtkOpenGLRenderer *) this->Renderers->GetNextItemAsObject();
	  ren != NULL;
	  ren = (vtkOpenGLRenderer *) this->Renderers->GetNextItemAsObject() )
      {
      ren->SetRenderWindow(NULL);
      }

    glFinish();
    glXDestroyContext( this->DisplayId, this->ContextId);
    this->ContextId = NULL;

    // then close the old window 
    if (this->OwnWindow && this->DisplayId && this->WindowId)
      {
      XDestroyWindow(this->DisplayId,this->WindowId);
      this->WindowId = (Window)NULL;
      }
    }

  this->TextureResourceIds->Delete();
}

// End the rendering process and display the image.
void vtkXOpenGLRenderWindow::Frame(void)
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
void vtkXOpenGLRenderWindow::SetStereoCapableWindow(int capable)
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
void vtkXOpenGLRenderWindow::WindowInitialize (void)
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
  
  this->Mapped = 1;
  
  // I do not know why the default size is not just set to 300 in the constructor.
  // But it needs to be set for the first render.
  this->Size[0] = width;
  this->Size[1] = height;

  // free the visual info
  if (v)
    {
    XFree(v);
    }
}

// Initialize the rendering window.
void vtkXOpenGLRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->ContextId)
    return;

  // now initialize the window 
  this->WindowInitialize();
}

// Change the window to fill the entire screen.
void vtkXOpenGLRenderWindow::SetFullScreen(int arg)
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

// Set the preferred window size to full screen.
void vtkXOpenGLRenderWindow::PrefFullScreen()
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

// Resize the window.
void vtkXOpenGLRenderWindow::WindowRemap()
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

// Begin the rendering process.
void vtkXOpenGLRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}

/*
void vtkXOpenGLRenderWindow::SetPosition(int x,int y)
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


// Specify the size of the rendering window.
void vtkXOpenGLRenderWindow::SetSize(int x,int y)
{
  if ((this->Size[0] != x)||(this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    }
  
  // if we arent mappen then just set the ivars 
  if (!this->Mapped)
    {
    return;
    }

  XResizeWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}



int vtkXOpenGLRenderWindow::GetDesiredDepth()
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
Visual *vtkXOpenGLRenderWindow::GetDesiredVisual ()
{
  XVisualInfo *v;
  Visual *vis;
  
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
Colormap vtkXOpenGLRenderWindow::GetDesiredColormap ()
{
  XVisualInfo *v;
  
  if (this->ColorMap) return this->ColorMap;
  
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

void vtkXOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkOpenGLRenderWindow::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
}

// the following can be useful for debugging XErrors
// When uncommented (along with the lines in MakeCurrent) 
// it will cause a segfault upon an XError instead of 
// the normal XError handler
//extern "C" {int vtkXError(Display *display, XErrorEvent *err)
//{
  // cause a segfault
//  *(float *)(0x01) = 1.0;
//  return 1;
//}}

void vtkXOpenGLRenderWindow::MakeCurrent()
{
  // when debugging XErrors uncomment the following lines
  //if (this->DisplayId)
  //  {
  //    XSynchronize(this->DisplayId,1);
  //  }
  //XSetErrorHandler(vtkXError);

  // set the current window 
  if (this->ContextId && (this->ContextId != glXGetCurrentContext()))
    {
    glXMakeCurrent(this->DisplayId,this->WindowId,this->ContextId);
    }
}




