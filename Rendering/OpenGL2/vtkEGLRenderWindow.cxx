/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkEGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"

#include "vtkToolkits.h"

#ifdef VTK_OPENGL_HAS_OSMESA
#include <GL/osmesa.h>
#endif

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"

#include "vtksys/SystemTools.hxx"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkEGLRenderWindow);

vtkEGLRenderWindow::vtkEGLRenderWindow()
{
  this->Display = EGL_NO_DISPLAY;
  this->Context = EGL_NO_CONTEXT;
  this->Surface = EGL_NO_SURFACE;
  this->OwnWindow = 1;
}

// free up memory & close the window
vtkEGLRenderWindow::~vtkEGLRenderWindow()
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
}

// End the rendering process and display the image.
void vtkEGLRenderWindow::Frame()
{
  this->MakeCurrent();

  if (this->OwnWindow)
    {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers
        && this->Display != EGL_NO_DISPLAY)
      {
      eglSwapBuffers(this->Display, this->Surface);
      vtkDebugMacro(<< " eglSwapBuffers\n");
      }
    else
      {
      glFlush();
      }
    }
  else
    {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
      {
      eglSwapBuffers( eglGetCurrentDisplay(), eglGetCurrentSurface( EGL_DRAW ) );
      vtkDebugMacro(<< " eglSwapBuffers\n");
      }
    else
      {
      glFlush();
      }
    }
}

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkEGLRenderWindow::SetStereoCapableWindow(int capable)
{
  if (this->Display == EGL_NO_DISPLAY)
    {
    vtkOpenGLRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

void vtkEGLRenderWindow::CreateAWindow()
{
  /*
   * Here specify the attributes of the desired configuration.
   * Below, we select an EGLConfig with at least 8 bits per color
   * component compatible with on-screen windows
   */
  const EGLint attribs[] = {
          EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_DEPTH_SIZE, 8,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
          EGL_NONE
  };
  const EGLint context_attribs[] = {
          EGL_CONTEXT_CLIENT_VERSION, 2,
          EGL_NONE
  };

  EGLint w, h, dummy, format;
  EGLint numConfigs;
  EGLConfig config;

  this->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  eglInitialize(this->Display, 0, 0);

  /* Here, the application chooses the configuration it desires. In this
   * sample, we have a very simplified selection process, where we pick
   * the first EGLConfig that matches our criteria */
  eglChooseConfig(this->Display, attribs, &config, 1, &numConfigs);

  /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
   * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
   * As soon as we picked a EGLConfig, we can safely reconfigure the
   * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
  eglGetConfigAttrib(this->Display, config, EGL_NATIVE_VISUAL_ID, &format);

  ANativeWindow_setBuffersGeometry(this->Window, 0, 0, format);

  this->Surface = eglCreateWindowSurface(this->Display, config, this->Window, NULL);
  this->Context = eglCreateContext(this->Display, config, NULL, context_attribs);

  this->Mapped = 1;
  this->OwnWindow = 1;

  this->MakeCurrent();

  eglQuerySurface(this->Display, this->Surface, EGL_WIDTH, &w);
  eglQuerySurface(this->Display, this->Surface, EGL_HEIGHT, &h);

  this->Size[0] = w;
  this->Size[1] = h;
}

void vtkEGLRenderWindow::DestroyWindow()
{
  this->ReleaseGraphicsResources();
  if (this->OwnWindow && this->Mapped && this->Display != EGL_NO_DISPLAY)
    {
    // make sure all other code knows we're not mapped anymore
    this->Mapped = 0;
    eglMakeCurrent(this->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (this->Context != EGL_NO_CONTEXT)
      {
      eglDestroyContext(this->Display, this->Context);
      this->Context = EGL_NO_CONTEXT;
      }
    if (this->Surface != EGL_NO_SURFACE)
      {
      eglDestroySurface(this->Display, this->Surface);
      this->Surface = EGL_NO_SURFACE;
      }
    eglTerminate(this->Display);
    this->Display = EGL_NO_DISPLAY;
    }
}

void vtkEGLRenderWindow::CreateOffScreenWindow(int width, int height)
{
}

void vtkEGLRenderWindow::DestroyOffScreenWindow()
{
}

void vtkEGLRenderWindow::ResizeOffScreenWindow(int width, int height)
{
}


// Initialize the window for rendering.
void vtkEGLRenderWindow::WindowInitialize (void)
{
  if (this->OwnWindow)
    {
    this->CreateAWindow();
    }

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
void vtkEGLRenderWindow::Initialize (void)
{
  if (this->Display == EGL_NO_DISPLAY)
    {
    // initialize the window
    this->WindowInitialize();
    }
}

void vtkEGLRenderWindow::Finalize (void)
{
  // clean and destroy window
  this->DestroyWindow();
}

// Change the window to fill the entire screen.
void vtkEGLRenderWindow::SetFullScreen(int arg)
{
  // window is always full screen
}

// Set the preferred window size to full screen.
void vtkEGLRenderWindow::PrefFullScreen()
{
  // don't show borders
  this->Borders = 0;
}

// Resize the window.
void vtkEGLRenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set everything up again
  this->Initialize();
}

// Begin the rendering process.
void vtkEGLRenderWindow::Start(void)
{
  this->Initialize();

  // set the current window
  this->MakeCurrent();
}

// Specify the size of the rendering window.
void vtkEGLRenderWindow::SetSize(int width,int height)
{
  this->Size[0] = width;
  this->Size[1] = height;
}

void vtkEGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  os << indent << "Display: " << this->Display << "\n";
  os << indent << "Surface: " << this->Surface << "\n";
}

void vtkEGLRenderWindow::MakeCurrent()
{
  if(this->Mapped &&
     this->Display != EGL_NO_DISPLAY &&
     this->Context != EGL_NO_CONTEXT &&
     this->Surface != EGL_NO_SURFACE)
    {
    if (eglMakeCurrent(this->Display, this->Surface, this->Surface, this->Context) == EGL_FALSE)
      {
      vtkErrorMacro("Unable to eglMakeCurrent");
      return;
      }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkEGLRenderWindow::IsCurrent()
{
  return true;
}

// Get the size of the screen in pixels
int *vtkEGLRenderWindow::GetScreenSize()
{
  if(this->Display != EGL_NO_DISPLAY)
    {
    EGLint w, h;
    eglQuerySurface(this->Display, this->Surface, EGL_WIDTH, &w);
    eglQuerySurface(this->Display, this->Surface, EGL_HEIGHT, &h);
    this->ScreenSize[0] = w;
    this->ScreenSize[1] = h;
    }

  return this->ScreenSize;
}

// Get the position in screen coordinates (pixels) of the window.
int *vtkEGLRenderWindow::GetPosition(void)
{
  return this->Position;
}

// Move the window to a new position on the display.
void vtkEGLRenderWindow::SetPosition(int x, int y)
{
}

int vtkEGLRenderWindow::SupportsOpenGL()
{
  this->MakeCurrent();
  if(this->Display == EGL_NO_DISPLAY && this->OwnWindow)
    {
    return false;
    }
  return true;
}

// Set this RenderWindow to a pre-existing window.
void vtkEGLRenderWindow::SetWindowInfo(char *info)
{
  this->OwnWindow = 0;
  this->Mapped = 1;
}


void vtkEGLRenderWindow::SetWindowName(const char * cname)
{
}

void vtkEGLRenderWindow::Render()
{
  // Now do the superclass stuff
  this->vtkOpenGLRenderWindow::Render();
}

//----------------------------------------------------------------------------
void vtkEGLRenderWindow::HideCursor()
{
}

//----------------------------------------------------------------------------
void vtkEGLRenderWindow::ShowCursor()
{
}

void vtkEGLRenderWindow::SetOffScreenRendering(int i)
{
}
