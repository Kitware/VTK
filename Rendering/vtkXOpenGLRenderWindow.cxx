/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXOpenGLRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef VTK_IMPLEMENT_MESA_CXX
#include "vtkXOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include <GL/gl.h>
#include "GL/glx.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "vtkToolkits.h"
#ifdef VTK_OPENGL_HAS_OSMESA
#include "MangleMesaInclude/osmesa.h"
#endif

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"

class vtkXOpenGLRenderWindow;
class vtkRenderWindow;
class vtkXOpenGLRenderWindowInternal
{
  friend class vtkXOpenGLRenderWindow;
private:
  vtkXOpenGLRenderWindowInternal(vtkRenderWindow*);

  GLXContext ContextId;

#if defined( VTK_OPENGL_HAS_OSMESA )
  // OffScreen stuff
  OSMesaContext OffScreenContextId;
  void *OffScreenWindow;
  int ScreenMapped;
  // Looks like this just stores DoubleBuffer.
  int ScreenDoubleBuffer;
#endif  
};

vtkXOpenGLRenderWindowInternal::vtkXOpenGLRenderWindowInternal(
  vtkRenderWindow *rw)
{
  this->ContextId = NULL;
  
  // OpenGL specific
#ifdef VTK_OPENGL_HAS_OSMESA
  this->OffScreenContextId = NULL;
  this->OffScreenWindow = NULL;
  this->ScreenMapped = rw->GetMapped();
  this->ScreenDoubleBuffer = rw->GetDoubleBuffer();
#endif  
}


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkXOpenGLRenderWindow, "1.16");
vtkStandardNewMacro(vtkXOpenGLRenderWindow);
#endif



#define MAX_LIGHTS 8

#ifdef VTK_OPENGL_HAS_OSMESA
// a couple of routines for offscreen rendering
void vtkOSMesaDestroyWindow(void *Window) 
{
  free(Window);
}

void *vtkOSMesaCreateWindow(int width, int height) 
{
  return malloc(width*height*4);
}
#endif




XVisualInfo *vtkXOpenGLRenderWindowTryForVisual(Display *DisplayId,
                                               int doublebuff, int stereo,
                                               int multisamples)
{
  int           index;
  static int    attributes[50];

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
  this->ParentId = (Window)NULL;
  this->ScreenSize[0] = 0;
  this->ScreenSize[1] = 0;
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)0;
  this->OwnWindow = 0;
 
  this->Internal = new vtkXOpenGLRenderWindowInternal(this);
}

// free up memory & close the window
vtkXOpenGLRenderWindow::~vtkXOpenGLRenderWindow()
{
  GLuint id;
  short cur_light;
  vtkOpenGLRenderer *ren;
  
  // make sure we have been initialized 
  if (this->Internal->ContextId 
#ifdef VTK_OPENGL_HAS_OSMESA
      || this->Internal->OffScreenContextId
#endif
    )
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
    for ( ren = vtkOpenGLRenderer::SafeDownCast(this->Renderers->GetNextItemAsObject());
          ren != NULL;
          ren = vtkOpenGLRenderer::SafeDownCast(this->Renderers->GetNextItemAsObject())  )
      {
      ren->SetRenderWindow(NULL);
      }

    glFinish();
#ifdef VTK_OPENGL_HAS_OSMESA
    if (this->OffScreenRendering && this->Internal->OffScreenContextId)
      {
      OSMesaDestroyContext(this->Internal->OffScreenContextId);
      this->Internal->OffScreenContextId = NULL;
      vtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
      this->Internal->OffScreenWindow = NULL;
      }
    else
#endif
      {
      glXDestroyContext( this->DisplayId, this->Internal->ContextId);
      this->Internal->ContextId = NULL;
      
      // then close the old window 
      if (this->OwnWindow && this->DisplayId && this->WindowId)
        {
        XDestroyWindow(this->DisplayId,this->WindowId);
        this->WindowId = (Window)NULL;
        }
      }
    }

  if (this->DisplayId)
    {
    XSync(this->DisplayId,0);
    }
  // if we create the display, we'll delete it
  if (this->OwnDisplay && this->DisplayId)
    {
    XCloseDisplay(this->DisplayId);
    this->DisplayId = NULL;
    }
  delete this->Internal;
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
    vtkOpenGLRenderWindow::SetStereoCapableWindow(capable);
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
  XSetWindowAttributes  attr;
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

  if ( ! this->OffScreenRendering)
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
    
    this->Internal->ContextId = glXCreateContext(this->DisplayId, v, 0, GL_TRUE);
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
#ifdef VTK_OPENGL_HAS_OSMESA
    if (!this->Internal->OffScreenWindow)
      {
      this->Internal->OffScreenWindow = vtkOSMesaCreateWindow(width,height);
      this->Size[0] = width;
      this->Size[1] = height;      
      this->OwnWindow = 1;
      }    
    this->Internal->OffScreenContextId = OSMesaCreateContext(GL_RGBA, NULL);
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
void vtkXOpenGLRenderWindow::Initialize (void)
{
  // make sure we havent already been initialized 
  if (this->Internal->ContextId 
#ifdef VTK_OPENGL_HAS_OSMESA
      || this->Internal->OffScreenContextId
#endif
      )
    {
    return;
    }

  // now initialize the window 
  this->WindowInitialize();
}

// Change the window to fill the entire screen.
void vtkXOpenGLRenderWindow::SetFullScreen(int arg)
{
  int *temp;
  
  if (this->OffScreenRendering)
    {
    return;
    }
  
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
void vtkXOpenGLRenderWindow::WindowRemap()
{
  short cur_light;

  /* first delete all the old lights */
  for (cur_light = GL_LIGHT0; cur_light < GL_LIGHT0+MAX_LIGHTS; cur_light++)
    {
    glDisable((GLenum)cur_light);
    }
#ifdef VTK_OPENGL_HAS_OSMESA
  if (this->OffScreenRendering && this->Internal->OffScreenContextId)
    {
    OSMesaDestroyContext(this->Internal->OffScreenContextId);
    this->Internal->OffScreenContextId = NULL;
    vtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
    this->Internal->OffScreenWindow = NULL;
    }
  else
#endif
    {
    glXDestroyContext( this->DisplayId, this->Internal->ContextId);
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

// Begin the rendering process.
void vtkXOpenGLRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->Internal->ContextId
#ifdef VTK_OPENGL_HAS_OSMESA
 && !this->Internal->OffScreenContextId
#endif
      )
    {
    this->Initialize();
    }

  // set the current window 
  this->MakeCurrent();
}


// Specify the size of the rendering window.
void vtkXOpenGLRenderWindow::SetSize(int x,int y)
{
  if ((this->Size[0] != x)||(this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    }

  if (this->OffScreenRendering 
#ifdef VTK_OPENGL_HAS_OSMESA
      && this->Internal->OffScreenWindow
#endif
      )
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
    
#ifdef VTK_OPENGL_HAS_OSMESA
    OSMesaDestroyContext(this->Internal->OffScreenContextId);
    this->Internal->OffScreenContextId = NULL;
    vtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
    this->Internal->OffScreenWindow = NULL;      
#endif
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
    // if we arent mappen then just set the ivars 
    if (!this->Mapped)
      {
      return;
      }
    
    XResizeWindow(this->DisplayId,this->WindowId,x,y);
    XSync(this->DisplayId,False);
    }
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
  Visual *vis=0;
  
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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->Internal->ContextId << "\n";
#ifdef VTK_OPENGL_HAS_OSMESA
  os << indent << "OffScreenContextId: " << this->Internal->OffScreenContextId << "\n";
#endif
  os << indent << "Color Map: " << this->ColorMap << "\n";
  os << indent << "Display Id: " << this->GetDisplayId() << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->GetWindowId() << "\n";
}

// the following can be useful for debugging XErrors
// When uncommented (along with the lines in MakeCurrent) 
// it will cause a segfault upon an XError instead of 
// the normal XError handler
//  extern "C" {int vtkXError(Display *display, XErrorEvent *err)
//  {
//  // cause a segfault
//    *(float *)(0x01) = 1.0;
//    return 1;
//  }}

void vtkXOpenGLRenderWindow::MakeCurrent()
{
  // when debugging XErrors uncomment the following lines
//    if (this->DisplayId)
//      {
//        XSynchronize(this->DisplayId,1);
//      }
//     XSetErrorHandler(vtkXError);
#ifdef VTK_OPENGL_HAS_OSMESA
  // set the current window 
  if (this->OffScreenRendering)
    {
    if (this->Internal->OffScreenContextId) 
      {
      if (OSMesaMakeCurrent(this->Internal->OffScreenContextId, 
                            this->Internal->OffScreenWindow, GL_UNSIGNED_BYTE, 
                            this->Size[0], this->Size[1]) != GL_TRUE) 
        {
        vtkWarningMacro("failed call to OSMesaMakeCurrent");
        }
      }
    }
  else
#endif
    {
    if (this->Internal->ContextId && (this->Internal->ContextId != glXGetCurrentContext()))
      {
      glXMakeCurrent(this->DisplayId,this->WindowId,this->Internal->ContextId);
      }
    }
}




int vtkXOpenGLRenderWindowFoundMatch;

Bool vtkXOpenGLRenderWindowPredProc(Display *vtkNotUsed(disp), XEvent *event, 
                              char *arg)
{
  Window win = (Window)arg;
  
  if (((reinterpret_cast<XAnyEvent *>(event))->window == win) &&
      ((event->type == ButtonPress)))
    vtkXOpenGLRenderWindowFoundMatch = 1;

  return 0;
  
}

void *vtkXOpenGLRenderWindow::GetGenericContext()
{
#if defined(MESA) && defined(VTK_OPENGL_HAS_OSMESA)
  if (this->OffScreenRendering)
    {
    return (void *)this->Internal->OffScreenContextId;
    }
  else
#endif
    {
    static GC gc = (GC) NULL;
    if (!gc) gc = XCreateGC(this->DisplayId, this->WindowId, 0, 0);
    return (void *) gc;
    }

}

int vtkXOpenGLRenderWindow::GetEventPending()
{
  XEvent report;
  
  vtkXOpenGLRenderWindowFoundMatch = 0;
  XCheckIfEvent(this->DisplayId, &report, vtkXOpenGLRenderWindowPredProc, 
                (char *)this->WindowId);
  return vtkXOpenGLRenderWindowFoundMatch;
}

// Get the size of the screen in pixels
int *vtkXOpenGLRenderWindow::GetScreenSize()
{
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  this->ScreenSize[0] = 
    DisplayWidth(this->DisplayId, DefaultScreen(this->DisplayId));
  this->ScreenSize[1] = 
    DisplayHeight(this->DisplayId, DefaultScreen(this->DisplayId));

  return this->ScreenSize;
}

// Get the position in screen coordinates (pixels) of the window.
int *vtkXOpenGLRenderWindow::GetPosition(void)
{
  XWindowAttributes attribs;
  int x,y;
  Window child;
  
  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);
  x = attribs.x;
  y = attribs.y;

  XTranslateCoordinates(this->DisplayId,this->WindowId,
                 RootWindowOfScreen(ScreenOfDisplay(this->DisplayId,0)),
                        x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

// Get this RenderWindow's X display id.
Display *vtkXOpenGLRenderWindow::GetDisplayId()
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
  vtkDebugMacro(<< "Returning DisplayId of " << (void *)this->DisplayId << "\n"); 

  return this->DisplayId;
}

// Get this RenderWindow's parent X window id.
Window vtkXOpenGLRenderWindow::GetParentId()
{
  vtkDebugMacro(<< "Returning ParentId of " << (void *)this->ParentId << "\n");
  return this->ParentId;
}

// Get this RenderWindow's X window id.
Window vtkXOpenGLRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << (void *)this->WindowId << "\n");
  return this->WindowId;
}

// Move the window to a new position on the display.
void vtkXOpenGLRenderWindow::SetPosition(int x, int y)
{
  // if we aren't mapped then just set the ivars
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

  XMoveWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}

// Sets the parent of the window that WILL BE created.
void vtkXOpenGLRenderWindow::SetParentId(Window arg)
{
  if (this->ParentId)
    {
    vtkErrorMacro("ParentId is already set.");
    return;
    }
  
  vtkDebugMacro(<< "Setting ParentId to " << (void *)arg << "\n"); 

  this->ParentId = arg;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXOpenGLRenderWindow::SetWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << (void *)arg << "\n"); 

  this->WindowId = arg;

  if (this->CursorHidden)
    {
    this->CursorHidden = 0;
    this->HideCursor();
    } 
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXOpenGLRenderWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  sscanf(info,"%i",&tmp);
 
  this->SetWindowId(tmp);
}

// Sets the X window id of the window that WILL BE created.
void vtkXOpenGLRenderWindow::SetParentInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  sscanf(info,"%i",&tmp);
 
  this->SetParentId(tmp);
}

void vtkXOpenGLRenderWindow::SetWindowId(void *arg)
{
  this->SetWindowId((Window)arg);
}
void vtkXOpenGLRenderWindow::SetParentId(void *arg)
{
  this->SetParentId((Window)arg);
}


void vtkXOpenGLRenderWindow::SetWindowName(char * name)
{
  XTextProperty win_name_text_prop;

  vtkOpenGLRenderWindow::SetWindowName( name );

  if (this->Mapped)
    {
    if( XStringListToTextProperty( &name, 1, &win_name_text_prop ) == 0 )
      {
      XFree (win_name_text_prop.value);
      vtkWarningMacro(<< "Can't rename window"); 
      return;
      }
    
    XSetWMName( this->DisplayId, this->WindowId, &win_name_text_prop );
    XSetWMIconName( this->DisplayId, this->WindowId, &win_name_text_prop );
    XFree (win_name_text_prop.value);
    }
}


// Specify the X window id to use if a WindowRemap is done.
void vtkXOpenGLRenderWindow::SetNextWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << (void *)arg << "\n"); 

  this->NextWindowId = arg;
}

// Set the X display id for this RenderWindow to use to a pre-existing 
// X display id.
void vtkXOpenGLRenderWindow::SetDisplayId(Display  *arg)
{
  vtkDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
  this->OwnDisplay = 0;
}
void vtkXOpenGLRenderWindow::SetDisplayId(void *arg)
{
  this->SetDisplayId((Display *)arg);
  this->OwnDisplay = 0;
}

void vtkXOpenGLRenderWindow::Render()
{
  XWindowAttributes attribs;

  // To avoid the expensive XGetWindowAttributes call,
  // compute size at the start of a render and use
  // the ivar other times.
  if (this->Mapped)
    {
    //  Find the current window size 
    XGetWindowAttributes(this->DisplayId, 
                                    this->WindowId, &attribs);

    this->Size[0] = attribs.width;
    this->Size[1] = attribs.height;
    }

  // Now do the superclass stuff
  this->vtkOpenGLRenderWindow::Render();
}

//----------------------------------------------------------------------------
void vtkXOpenGLRenderWindow::HideCursor()
{
  static char blankBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  
  static XColor black = { 0, 0, 0, 0, 0, 0 };
 
  if (!this->DisplayId || !this->WindowId)
    {
    this->CursorHidden = 1;
    }
  else if (!this->CursorHidden)
    {
    Pixmap blankPixmap = XCreateBitmapFromData(this->DisplayId,
                                               this->WindowId,
                                               blankBits, 16, 16);
    
    Cursor blankCursor = XCreatePixmapCursor(this->DisplayId, blankPixmap,
                                             blankPixmap, &black, &black,
                                             7, 7);
    
    XDefineCursor(this->DisplayId, this->WindowId, blankCursor);
    
    XFreePixmap(this->DisplayId, blankPixmap);
    
    this->CursorHidden = 1;
    }
}

//----------------------------------------------------------------------------
void vtkXOpenGLRenderWindow::ShowCursor()
{
  if (!this->DisplayId || !this->WindowId)
    {
    this->CursorHidden = 0;
    }
  else if (this->CursorHidden)
    {
    XUndefineCursor(this->DisplayId, this->WindowId);
    this->CursorHidden = 0;
    }
}                                  


//============================================================================
// Stuff above this is almost a mirror of vtkXOpenGLRenderWindow.
// The code specific to OpenGL Off-Screen stuff may eventually be 
// put in a supper class so this whole file could just be included 
// (mangled) from vtkXOpenGLRenderWindow like the other OpenGL classes.
//============================================================================

void vtkXOpenGLRenderWindow::SetOffScreenRendering(int i)
{
  if (this->OffScreenRendering == i)
    {
    return;
    }
  
#ifdef VTK_OPENGL_HAS_OSMESA
  // invoke super
  this->vtkRenderWindow::SetOffScreenRendering(i);
  
  // setup everything
  if (i)
    {
    this->Internal->ScreenDoubleBuffer = this->DoubleBuffer;    
    this->DoubleBuffer = 0;
    this->Internal->ScreenMapped = this->Mapped;
    this->Mapped = 0;
    if (!this->Internal->OffScreenWindow)
      {
      this->WindowInitialize();
      }    
    }
  else
    {
    if (this->Internal->OffScreenWindow)
      {
      OSMesaDestroyContext(this->Internal->OffScreenContextId);
      this->Internal->OffScreenContextId = NULL;
      vtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
      this->Internal->OffScreenWindow = NULL;      
      }
    this->DoubleBuffer = this->Internal->ScreenDoubleBuffer;
    this->Mapped = this->Internal->ScreenMapped;
    this->MakeCurrent();
    // reset the size based on the screen window
    this->GetSize();
    this->WindowInitialize();
    }
#endif
}

// This probably has been moved to superclass.
void *vtkXOpenGLRenderWindow::GetGenericWindowId()
{
#ifdef VTK_OPENGL_HAS_OSMESA
  if (this->OffScreenRendering)
    {
    return (void *)this->Internal->OffScreenWindow;
    }
#endif
  return (void *)this->WindowId;
}

