/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkIOSRenderWindow.mm

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRenderWindow.h"

#import "vtkIOSRenderWindow.h"
#import "vtkRenderWindowInteractor.h"
#import "vtkCommand.h"
#import "vtkIdList.h"
#import "vtkObjectFactory.h"
#import "vtkRendererCollection.h"

#import <sstream>

#include "vtk_glew.h"

vtkStandardNewMacro(vtkIOSRenderWindow);


//----------------------------------------------------------------------------
vtkIOSRenderWindow::vtkIOSRenderWindow()
{
  this->WindowCreated = 0;
  this->ViewCreated = 0;
  this->SetWindowName("Visualization Toolkit - IOS");
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->OnScreenInitialized = 0;
  this->OffScreenInitialized = 0;
  // it seems that LEFT/RIGHT cause issues on IOS so we just use
  // generic BACK/FRONT
  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT);
}

//----------------------------------------------------------------------------
vtkIOSRenderWindow::~vtkIOSRenderWindow()
{
  if (this->CursorHidden)
  {
    this->ShowCursor();
  }
  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
  {
    ren->SetRenderWindow(NULL);
  }

  this->SetContextId(NULL);
  this->SetPixelFormat(NULL);
  this->SetRootWindow(NULL);
  this->SetWindowId(NULL);
  this->SetParentId(NULL);
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::Finalize()
{
  if(this->OffScreenInitialized)
  {
    this->OffScreenInitialized = 0;
    this->DestroyOffScreenWindow();
  }
  if(this->OnScreenInitialized)
  {
    this->OnScreenInitialized = 0;
    this->DestroyWindow();
  }
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::DestroyWindow()
{
  // finish OpenGL rendering
  if (this->OwnContext && this->GetContextId())
  {
    this->MakeCurrent();
    this->ReleaseGraphicsResources(this);
  }
  this->SetContextId(NULL);
  this->SetPixelFormat(NULL);

  this->SetWindowId(NULL);
  this->SetParentId(NULL);
  this->SetRootWindow(NULL);
}

int vtkIOSRenderWindow::ReadPixels(
  const vtkRecti& rect, int front, int glFormat, int glType, void* data,
  int right)
{
  if (glFormat != GL_RGB || glType != GL_UNSIGNED_BYTE)
  {
    return this->Superclass::ReadPixels(rect, front, glFormat, glType, data,
                                        right);
  }

  // iOS has issues with getting RGB so we get RGBA
  unsigned char* uc4data = new unsigned char[rect.GetWidth() * rect.GetHeight() * 4];
  int retVal = this->Superclass::ReadPixels(rect, front, GL_RGBA, GL_UNSIGNED_BYTE, uc4data, right);

  unsigned char* dPtr = reinterpret_cast<unsigned char*>(data);
  const unsigned char* lPtr = uc4data;
  for (int i = 0, height = rect.GetHeight(); i < height; i++)
  {
    for (int j = 0, width = rect.GetWidth(); j < width; j++)
    {
      *(dPtr++) = *(lPtr++);
      *(dPtr++) = *(lPtr++);
      *(dPtr++) = *(lPtr++);
      lPtr++;
    }
  }
  delete[] uc4data;
  return retVal;
}


//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetWindowName( const char * _arg )
{
  vtkWindow::SetWindowName(_arg);
}

//----------------------------------------------------------------------------
bool vtkIOSRenderWindow::InitializeFromCurrentContext()
{
  // NSOpenGLContext *currentContext = [NSOpenGLContext currentContext];
  // if (currentContext != NULL)
  //   {
  //   UIView *currentView = [currentContext view];
  //   if (currentView != NULL)
  //     {
  //     UIWindow *window = [currentView window];
  //     this->SetWindowId(currentView);
  //     this->SetRootWindow(window);
  //     this->SetContextId((void*)currentContext);
  //     this->OpenGLInit();
  //     this->OwnContext = 0;
  //     return true;
  //     }
  //   }
  return false;
}

//----------------------------------------------------------------------------
int vtkIOSRenderWindow::GetEventPending()
{
  return 0;
}

//----------------------------------------------------------------------------
// Initialize the rendering process.
void vtkIOSRenderWindow::Start()
{
  this->Initialize();

  // set the current window
  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::MakeCurrent()
{
  // if (this->GetContextId())
  //   {
  //   [(NSOpenGLContext*)this->GetContextId() makeCurrentContext];
  //   }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkIOSRenderWindow::IsCurrent()
{
  return true;
}

//----------------------------------------------------------------------------
bool vtkIOSRenderWindow::IsDrawable()
{
  // you must initialize it first
  // else it always evaluates false
  this->Initialize();

  return true;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::UpdateContext()
{
}

//----------------------------------------------------------------------------
const char* vtkIOSRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  const char* glVendor = (const char*) glGetString(GL_VENDOR);
  const char* glRenderer = (const char*) glGetString(GL_RENDERER);
  const char* glVersion = (const char*) glGetString(GL_VERSION);
  const char* glExtensions = (const char*) glGetString(GL_EXTENSIONS);

  std::ostringstream strm;
  strm << "OpenGL vendor string:  " << glVendor
       << "\nOpenGL renderer string:  " << glRenderer
       << "\nOpenGL version string:  " << glVersion
       << "\nOpenGL extensions:  " << glExtensions << endl;

  delete[] this->Capabilities;

  size_t len = strm.str().length() + 1;
  this->Capabilities = new char[len];
  strlcpy(this->Capabilities, strm.str().c_str(), len);

  return this->Capabilities;
}

//----------------------------------------------------------------------------
int vtkIOSRenderWindow::SupportsOpenGL()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkIOSRenderWindow::IsDirect()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetSize(int* a)
{
  this->SetSize( a[0], a[1] );
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y) || (this->GetParentId()))
  {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
  }
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetPosition(int* a)
{
  this->SetPosition( a[0], a[1] );
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y)
      || (this->GetParentId()))
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
  }
}

//----------------------------------------------------------------------------
// End the rendering process and display the image.
void vtkIOSRenderWindow::Frame()
{
  this->MakeCurrent();

  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
//    [(NSOpenGLContext*)this->GetContextId() flushBuffer];
  }
}

//----------------------------------------------------------------------------
// Specify various window parameters.
void vtkIOSRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetupPixelFormat(void*, void*, int, int, int)
{
  vtkErrorMacro(<< "vtkIOSRenderWindow::SetupPixelFormat - IMPLEMENT");
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetupPalette(void*)
{
  vtkErrorMacro(<< "vtkIOSRenderWindow::SetupPalette - IMPLEMENT");
}

//----------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkIOSRenderWindow::CreateAWindow()
{
  static unsigned count = 1;

  this->CreateGLContext();

  this->MakeCurrent();

  // wipe out any existing display lists
  vtkRenderer *renderer = NULL;
  vtkCollectionSimpleIterator rsit;

  for ( this->Renderers->InitTraversal(rsit);
        (renderer = this->Renderers->GetNextRenderer(rsit));)
  {
    renderer->SetRenderWindow(0);
    renderer->SetRenderWindow(this);
  }
  this->OpenGLInit();
  this->Mapped = 1;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::CreateGLContext()
{
}

//----------------------------------------------------------------------------
// Initialize the rendering window.
void vtkIOSRenderWindow::Initialize ()
{
  this->OpenGLInit();
  this->Mapped = 1;
}

//-----------------------------------------------------------------------------
void vtkIOSRenderWindow::DestroyOffScreenWindow()
{
}

//----------------------------------------------------------------------------
// Get the current size of the window.
int *vtkIOSRenderWindow::GetSize()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
  {
    return this->Superclass::GetSize();
  }

  return this->Superclass::GetSize();
}

//----------------------------------------------------------------------------
// Get the current size of the screen in pixels.
int *vtkIOSRenderWindow::GetScreenSize()
{
  return this->Size;
}

//----------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int *vtkIOSRenderWindow::GetPosition()
{
  return this->Position;
}

//----------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkIOSRenderWindow::SetFullScreen(int arg)
{
}

//----------------------------------------------------------------------------
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkIOSRenderWindow::SetStereoCapableWindow(int capable)
{
  if (this->GetContextId() == 0)
  {
    vtkRenderWindow::SetStereoCapableWindow(capable);
  }
  else
  {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
  }
}

//----------------------------------------------------------------------------
// Set the preferred window size to full screen.
void vtkIOSRenderWindow::PrefFullScreen()
{
  int *size = this->GetScreenSize();
  vtkWarningMacro(<< "Can only set FullScreen before showing window: "
                  << size[0] << 'x' << size[1] << ".");
}

//----------------------------------------------------------------------------
// Remap the window.
void vtkIOSRenderWindow::WindowRemap()
{
  vtkWarningMacro(<< "Can't remap the window.");
  // Acquire the display and capture the screen.
  // Create the full-screen window.
  // Add the context.
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MultiSamples: " << this->MultiSamples << endl;
  os << indent << "RootWindow (UIWindow): " << this->GetRootWindow() << endl;
  os << indent << "WindowId (UIView): " << this->GetWindowId() << endl;
  os << indent << "ParentId: " << this->GetParentId() << endl;
  os << indent << "ContextId: " << this->GetContextId() << endl;
  os << indent << "PixelFormat: " << this->GetPixelFormat() << endl;
  os << indent << "WindowCreated: " << (this->WindowCreated ? "Yes" : "No") << endl;
  os << indent << "ViewCreated: " << (this->ViewCreated ? "Yes" : "No") << endl;
}

//----------------------------------------------------------------------------
int vtkIOSRenderWindow::GetDepthBufferSize()
{
  if ( this->Mapped )
  {
    GLint size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
  }
  else
  {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
  }
}

//----------------------------------------------------------------------------
// Returns the UIWindow* associated with this vtkRenderWindow.
void *vtkIOSRenderWindow::GetRootWindow()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the UIWindow* associated with this vtkRenderWindow.
void vtkIOSRenderWindow::SetRootWindow(void *vtkNotUsed(arg))
{
}

//----------------------------------------------------------------------------
// Returns the UIView* associated with this vtkRenderWindow.
void *vtkIOSRenderWindow::GetWindowId()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the UIView* associated with this vtkRenderWindow.
void vtkIOSRenderWindow::SetWindowId(void *vtkNotUsed(arg))
{
}

//----------------------------------------------------------------------------
// Returns the UIView* that is the parent of this vtkRenderWindow.
void *vtkIOSRenderWindow::GetParentId()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the UIView* that this vtkRenderWindow should use as a parent.
void vtkIOSRenderWindow::SetParentId(void *vtkNotUsed(arg))
{
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLContext* associated with this vtkRenderWindow.
void vtkIOSRenderWindow::SetContextId(void *vtkNotUsed(contextId))
{
}

//----------------------------------------------------------------------------
// Returns the NSOpenGLContext* associated with this vtkRenderWindow.
void *vtkIOSRenderWindow::GetContextId()
{
  return NULL;
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLPixelFormat* associated with this vtkRenderWindow.
void vtkIOSRenderWindow::SetPixelFormat(void *vtkNotUsed(pixelFormat))
{
}

//----------------------------------------------------------------------------
// Returns the NSOpenGLPixelFormat* associated with this vtkRenderWindow.
void *vtkIOSRenderWindow::GetPixelFormat()
{
  return NULL;
}


//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetWindowInfo(char *info)
{
  // The parameter is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
  {
    (void)sscanf(info, "%tu", &tmp);
  }

  this->SetWindowId (reinterpret_cast<void *>(tmp));
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetParentInfo(char *info)
{
  // The parameter is an ASCII string of a decimal number representing
  // a pointer to the window. Convert it back to a pointer.
  ptrdiff_t tmp = 0;
  if (info)
  {
    (void)sscanf(info, "%tu", &tmp);
  }

  this->SetParentId (reinterpret_cast<void *>(tmp));
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::HideCursor()
{
  if (this->CursorHidden)
  {
    return;
  }
  this->CursorHidden = 1;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
  {
    return;
  }
  this->CursorHidden = 0;
}

// ---------------------------------------------------------------------------
int vtkIOSRenderWindow::GetWindowCreated()
{
  return this->WindowCreated;
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetCursorPosition(int x, int y)
{
}

//----------------------------------------------------------------------------
void vtkIOSRenderWindow::SetCurrentCursor(int shape)
{
  if (this->InvokeEvent(vtkCommand::CursorChangedEvent, &shape))
  {
    return;
  }
  this->Superclass::SetCurrentCursor(shape);
}
