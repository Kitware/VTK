/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOSOpenGLRenderWindow.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef VTK_IMPLEMENT_MESA_CXX
#include "vtkOSOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include <GL/gl.h>
// #include "GL/glx.h"
#include "vtkgl.h"
#else
#include "MangleMesaInclude/osmesa.h"
#endif

#include "vtkToolkits.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef VTK_OPENGL_HAS_OSMESA
#include <GL/osmesa.h>
#endif
#endif

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkOpenGLExtensionManager.h"

#include "vtksys/SystemTools.hxx"
#include "vtksys/ios/sstream"

class vtkOSOpenGLRenderWindow;
class vtkRenderWindow;

class vtkOSOpenGLRenderWindowInternal
{
  friend class vtkOSOpenGLRenderWindow;
private:
  vtkOSOpenGLRenderWindowInternal(vtkRenderWindow*);

  // store previous settings of on screen window
  int ScreenDoubleBuffer;
  int ScreenMapped;

  // OffScreen stuff
  OSMesaContext OffScreenContextId;
  void *OffScreenWindow;
};

vtkOSOpenGLRenderWindowInternal::vtkOSOpenGLRenderWindowInternal(
  vtkRenderWindow *rw)
{
  
  this->ScreenMapped = rw->GetMapped();
  this->ScreenDoubleBuffer = rw->GetDoubleBuffer();
  
  // OpenGL specific
  this->OffScreenContextId = NULL;
  this->OffScreenWindow = NULL;
}


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOSOpenGLRenderWindow);
#endif



#define MAX_LIGHTS 8

// a couple of routines for offscreen rendering
void vtkOSMesaDestroyWindow(void *Window) 
{
  free(Window);
}

void *vtkOSMesaCreateWindow(int width, int height) 
{
  return malloc(width*height*4);
}

vtkOSOpenGLRenderWindow::vtkOSOpenGLRenderWindow()
{
//   this->ParentId = (Window)NULL;
  this->ScreenSize[0] = 1280;
  this->ScreenSize[1] = 1024;
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->OwnWindow = 0;
 
  this->Internal = new vtkOSOpenGLRenderWindowInternal(this);

  this->Capabilities = 0;

}

// free up memory & close the window
vtkOSOpenGLRenderWindow::~vtkOSOpenGLRenderWindow()
{
  // close-down all system-specific drawing resources
  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    }

  delete this->Internal;
}

// End the rendering process and display the image.
void vtkOSOpenGLRenderWindow::Frame()
{
  this->MakeCurrent();
  glFlush();
}
 
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkOSOpenGLRenderWindow::SetStereoCapableWindow(int capable)
{
  if (!this->Internal->OffScreenContextId)
    {
    vtkOpenGLRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

void vtkOSOpenGLRenderWindow::CreateAWindow()
{
  this->CreateOffScreenWindow(this->ScreenSize[0], this->ScreenSize[1]);
}

void vtkOSOpenGLRenderWindow::DestroyWindow()
{
  this->MakeCurrent();
  
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  vtkRenderer* ren;
  this->Renderers->InitTraversal();
  for ( ren = vtkOpenGLRenderer::SafeDownCast(this->Renderers->GetNextItemAsObject());
        ren != NULL;
        ren = vtkOpenGLRenderer::SafeDownCast(this->Renderers->GetNextItemAsObject())  )
    {
    ren->SetRenderWindow(NULL);
    ren->SetRenderWindow(this);
    }


  if (this->Capabilities)
    {
    delete[] this->Capabilities;
    this->Capabilities = 0;
    }
    
  // make sure all other code knows we're not mapped anymore
  this->Mapped = 0;

}

void vtkOSOpenGLRenderWindow::CreateOffScreenWindow(int width, int height)
{
  
  this->DoubleBuffer = 0;

  if (!this->Internal->OffScreenWindow)
    {
    this->Internal->OffScreenWindow = vtkOSMesaCreateWindow(width,height);
    this->OwnWindow = 1;
    }
  if (!this->Internal->OffScreenContextId)
    {
    this->Internal->OffScreenContextId = OSMesaCreateContext(GL_RGBA, NULL);
    }
  this->MakeCurrent();

  this->Mapped = 0;
  this->Size[0] = width;
  this->Size[1] = height;
  
  this->MakeCurrent();

  // tell our renderers about us
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); 
       (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(0);
    ren->SetRenderWindow(this);
    }

  this->OpenGLInit();
}

void vtkOSOpenGLRenderWindow::DestroyOffScreenWindow()
{
  
  // release graphic resources.
  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    ren->SetRenderWindow(this);
    }


  if (this->Internal->OffScreenContextId)
    {
    OSMesaDestroyContext(this->Internal->OffScreenContextId);
    this->Internal->OffScreenContextId = NULL;
    vtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
    this->Internal->OffScreenWindow = NULL;
    }
}

void vtkOSOpenGLRenderWindow::ResizeOffScreenWindow(int width, int height)
{
  if(this->Internal->OffScreenContextId)
    {
    this->DestroyOffScreenWindow();
    this->CreateOffScreenWindow(width, height);
    }
}


// Initialize the window for rendering.
void vtkOSOpenGLRenderWindow::WindowInitialize (void)
{
  this->CreateAWindow();

  this->MakeCurrent();

  // tell our renderers about us
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); 
       (ren = this->Renderers->GetNextItem());)
    {
    ren->SetRenderWindow(0);
    ren->SetRenderWindow(this);
    }

  this->OpenGLInit();
}

// Initialize the rendering window.
void vtkOSOpenGLRenderWindow::Initialize (void)
{
  if(! (this->Internal->OffScreenContextId))
    {
    // initialize offscreen window
    int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
    int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
    this->CreateOffScreenWindow(width, height);
    }
}

void vtkOSOpenGLRenderWindow::Finalize (void)
{

  // clean up offscreen stuff
  this->SetOffScreenRendering(0);

  // clean and destroy window
  this->DestroyWindow();

}

// Change the window to fill the entire screen.
void vtkOSOpenGLRenderWindow::SetFullScreen(int arg)
{
  this->Modified();
}

// Resize the window.
void vtkOSOpenGLRenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set the default windowid 
//   this->WindowId = this->NextWindowId;
//   this->NextWindowId = (Window)NULL;

  // set everything up again 
  this->Initialize();
}

// Begin the rendering process.
void vtkOSOpenGLRenderWindow::Start(void)
{
  this->Initialize();

  // set the current window 
  this->MakeCurrent();
}


// Specify the size of the rendering window.
void vtkOSOpenGLRenderWindow::SetSize(int width,int height)
{
  if ((this->Size[0] != width)||(this->Size[1] != height))
    {
    this->Size[0] = width;
    this->Size[1] = height;
    this->ResizeOffScreenWindow(width,height);
    this->Modified();
    }
}

void vtkOSOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OffScreenContextId: " << this->Internal->OffScreenContextId << "\n";
//   os << indent << "Color Map: " << this->ColorMap << "\n";
//   os << indent << "Next Window Id: " << this->NextWindowId << "\n";
//   os << indent << "Window Id: " << this->GetWindowId() << "\n";
}

void vtkOSOpenGLRenderWindow::MakeCurrent()
{
  // set the current window 
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

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOSOpenGLRenderWindow::IsCurrent()
{
  bool result=false;
  if(this->Internal->OffScreenContextId)
    {
    result=this->Internal->OffScreenContextId==OSMesaGetCurrentContext();
    }
  return result;
}


void vtkOSOpenGLRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

void *vtkOSOpenGLRenderWindow::GetGenericContext()
{
  return (void *)this->Internal->OffScreenContextId;
}

int vtkOSOpenGLRenderWindow::GetEventPending()
{
  return 0;
}

// Get the size of the screen in pixels
int *vtkOSOpenGLRenderWindow::GetScreenSize()
{

  this->ScreenSize[0] = 1280;
  this->ScreenSize[1] = 1024;
  return this->ScreenSize;
}

// Get the position in screen coordinates (pixels) of the window.
int *vtkOSOpenGLRenderWindow::GetPosition(void)
{
  return this->Position;
}

// Move the window to a new position on the display.
void vtkOSOpenGLRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x)||(this->Position[1] != y))
    {
    this->Modified();
    }
  this->Position[0] = x;
  this->Position[1] = y;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkOSOpenGLRenderWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  this->OwnDisplay = 1;

  sscanf(info,"%i",&tmp);
 
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkOSOpenGLRenderWindow::SetNextWindowInfo(char *info)
{
  int tmp;
  sscanf(info,"%i",&tmp);
 
//   this->SetNextWindowId((Window)tmp);
}

// Sets the X window id of the window that WILL BE created.
void vtkOSOpenGLRenderWindow::SetParentInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  this->OwnDisplay = 1;

  sscanf(info,"%i",&tmp);
 
//   this->SetParentId(tmp);
}

void vtkOSOpenGLRenderWindow::SetWindowId(void *arg)
{
//   this->SetWindowId((Window)arg);
}
void vtkOSOpenGLRenderWindow::SetParentId(void *arg)
{
//   this->SetParentId((Window)arg);
}

const char* vtkOSOpenGLRenderWindow::ReportCapabilities()
{
  MakeCurrent();

//   int scrnum = DefaultScreen(this->DisplayId);
  
  const char *glVendor = (const char *) glGetString(GL_VENDOR);
  const char *glRenderer = (const char *) glGetString(GL_RENDERER);
  const char *glVersion = (const char *) glGetString(GL_VERSION);
  const char *glExtensions = (const char *) glGetString(GL_EXTENSIONS);

  vtksys_ios::ostringstream strm;
  strm << "OpenGL vendor string:  " << glVendor << endl;
  strm << "OpenGL renderer string:  " << glRenderer << endl;
  strm << "OpenGL version string:  " << glVersion << endl;
  strm << "OpenGL extensions:  " << glExtensions << endl;
  delete[] this->Capabilities;
  size_t len = strm.str().length();
  this->Capabilities = new char[len + 1];
  strncpy(this->Capabilities, strm.str().c_str(), len);
  this->Capabilities[len] = 0;
  return this->Capabilities;
}

int vtkOSOpenGLRenderWindow::SupportsOpenGL()
{
  MakeCurrent();
  return 1;
}


int vtkOSOpenGLRenderWindow::IsDirect()
{
  MakeCurrent();
  return 0;
}


void vtkOSOpenGLRenderWindow::SetWindowName(const char * cname)
{
  char *name = new char[ strlen(cname)+1 ];
  strcpy(name, cname);
  vtkOpenGLRenderWindow::SetWindowName( name );
  delete [] name;
}

// Specify the X window id to use if a WindowRemap is done.
/*void vtkOSOpenGLRenderWindow::SetNextWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << (void *)arg << "\n"); 

  this->NextWindowId = arg;
}*/

void vtkOSOpenGLRenderWindow::SetNextWindowId(void *arg)
{
//   this->SetNextWindowId((Window)arg);
}


// Set the X display id for this RenderWindow to use to a pre-existing 
// X display id.
/*void vtkOSOpenGLRenderWindow::SetDisplayId(Display  *arg)
{
  vtkDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
  this->OwnDisplay = 0;

}*/

/*void vtkOSOpenGLRenderWindow::SetDisplayId(void *arg)
{
  this->SetDisplayId((Display *)arg);
  this->OwnDisplay = 0;
}*/

//============================================================================
// Stuff above this is almost a mirror of vtkOSOpenGLRenderWindow.
// The code specific to OpenGL Off-Screen stuff may eventually be 
// put in a supper class so this whole file could just be included 
// (mangled) from vtkOSOpenGLRenderWindow like the other OpenGL classes.
//============================================================================

void vtkOSOpenGLRenderWindow::SetOffScreenRendering(int i)
{
  if (this->OffScreenRendering == i)
    {
    return;
    }

  // invoke super
  this->vtkRenderWindow::SetOffScreenRendering(i);

  this->Internal->ScreenDoubleBuffer = this->DoubleBuffer;
  this->DoubleBuffer = 0;
  if(this->Mapped)
    {
    this->DestroyWindow();
    }

  // delay initialization until Render
}

// This probably has been moved to superclass.
void *vtkOSOpenGLRenderWindow::GetGenericWindowId()
{
  return (void *)this->Internal->OffScreenWindow;
}

void vtkOSOpenGLRenderWindow::SetCurrentCursor(int shape)
{
  if ( this->InvokeEvent(vtkCommand::CursorChangedEvent,&shape) )
    {
    return;
    }
  this->Superclass::SetCurrentCursor(shape);
}
