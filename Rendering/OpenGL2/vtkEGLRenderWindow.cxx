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


#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"
#include "vtksys/SystemTools.hxx"
#include "vtkToolkits.h"
#include "vtk_glew.h"

#include <sstream>
#include <EGL/egl.h>

#if ANDROID
#include <android/native_window.h>
#endif


namespace
{
  typedef void* EGLDeviceEXT;
  typedef EGLBoolean (*EGLQueryDevicesType)(EGLint,EGLDeviceEXT*,EGLint*);
  typedef EGLDisplay (*EGLGetPlatformDisplayType)(EGLenum, void *, const EGLint *);
  const EGLenum EGL_PLATFORM_DEVICE_EXT = 0x313F;

  struct vtkEGLDeviceExtensions
  {
    static vtkEGLDeviceExtensions* GetInstance()
    {
      static vtkEGLDeviceExtensions* instance = NULL;
      if (instance == NULL)
      {
        instance = new vtkEGLDeviceExtensions();
      }
      return instance;
    }
    bool Available()
    {
      return this->Available_;
    }
    bool Available_;
    EGLQueryDevicesType eglQueryDevices;
    EGLGetPlatformDisplayType eglGetPlatformDisplay;

  private:
    vtkEGLDeviceExtensions()
    {
      this->Available_ = false;
      this->eglQueryDevices = NULL;
      this->eglGetPlatformDisplay = NULL;
      const char* s = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
      std::string platformExtensions(s);
      if (platformExtensions.find("EGL_EXT_device_base") != std::string::npos &&
          platformExtensions.find("EGL_EXT_platform_device") != std::string::npos &&
          platformExtensions.find("EGL_EXT_platform_base") != std::string::npos)
      {
        this->eglQueryDevices = (EGLQueryDevicesType)eglGetProcAddress("eglQueryDevicesEXT");
        this->eglGetPlatformDisplay = (EGLGetPlatformDisplayType)
            eglGetProcAddress("eglGetPlatformDisplayEXT");
        if(this->eglQueryDevices && this->eglGetPlatformDisplay)
        {
          this->Available_ = true;
        }
      }
    }
  };
};




vtkStandardNewMacro(vtkEGLRenderWindow);

struct vtkEGLRenderWindow::vtkInternals
{
  EGLNativeWindowType Window;
  EGLDisplay Display;
  EGLSurface Surface;
  EGLContext Context;
  vtkInternals() : Window((EGLNativeWindowType)0),
                   Display(EGL_NO_DISPLAY),
                   Surface(EGL_NO_SURFACE),
                   Context(EGL_NO_CONTEXT)
  {
  }
};

vtkEGLRenderWindow::vtkEGLRenderWindow()
{
  this->Internals = new vtkInternals();
  this->OwnWindow = 1;
  this->ScreenSize[0] = 1920;
  this->ScreenSize[1] = 1080;
  // this is initialized in vtkRenderWindow
  // so we don't need to initialize on else
#ifdef VTK_USE_OFFSCREEN_EGL
  this->DeviceIndex = VTK_EGL_DEVICE_INDEX;
#endif
#if ANDROID
  this->OffScreenRendering = false;
#else
  this->OffScreenRendering = true;
#endif
  this->IsPointSpriteBugTested = false;
  this->IsPointSpriteBugPresent_ = false;
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
  delete this->Internals;
}

// End the rendering process and display the image.
void vtkEGLRenderWindow::Frame()
{
  vtkInternals* impl = this->Internals;
  this->MakeCurrent();
  if (this->OwnWindow)
  {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers
        && impl->Display != EGL_NO_DISPLAY)
    {
      eglSwapBuffers(impl->Display, impl->Surface);
      vtkDebugMacro(<< " eglSwapBuffers\n");
    }
  }
  else
  {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
      eglSwapBuffers( eglGetCurrentDisplay(), eglGetCurrentSurface( EGL_DRAW ) );
      vtkDebugMacro(<< " eglSwapBuffers\n");
    }
  }
}

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkEGLRenderWindow::SetStereoCapableWindow(int capable)
{
  vtkInternals* impl = this->Internals;
  if (impl->Display == EGL_NO_DISPLAY)
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
  int s[2];
  if (this->Size[0] != 0 && this->Size[1] != 0)
  {
    s[0] = this->Size[0];
    s[1] = this->Size[1];
  }
  else
  {
    s[0] = this->ScreenSize[0];
    s[1] = this->ScreenSize[1];
  }
  this->ResizeWindow(s[0], s[1]);
}


int vtkEGLRenderWindow::GetNumberOfDevices()
{
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  if (ext->Available())
  {
      EGLint num_devices = 0;
      ext->eglQueryDevices(num_devices, NULL, &num_devices);
      return num_devices;
  }
  vtkWarningMacro("Getting the number of devices (graphics cards) on a system require "
                  "EGL_EXT_device_base, EGL_EXT_platform_device and EGL_EXT_platform_base extensions");
  return 0;
}


void vtkEGLRenderWindow::SetDeviceAsDisplay(int deviceIndex)
{
  vtkInternals* impl = this->Internals;
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  if (ext->Available())
  {
    EGLint num_devices = 0;
    ext->eglQueryDevices(num_devices, NULL, &num_devices);
    if (deviceIndex >= num_devices)
    {
      vtkWarningMacro("EGL device index: " << deviceIndex << " is greater than "
                      "the number of supported deviced in the system: " << num_devices <<
                      ". Using device 0 ...");
      return;
    }
    EGLDeviceEXT* devices = new EGLDeviceEXT[num_devices];
    ext->eglQueryDevices(num_devices, devices, &num_devices);
    impl->Display =
      ext->eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[deviceIndex], NULL);
    delete[] devices;
    return;
  }
  vtkWarningMacro("Setting an EGL display to device index: " << deviceIndex << " require "
                  "EGL_EXT_device_base EGL_EXT_platform_device EGL_EXT_platform_base extensions");
}


void vtkEGLRenderWindow::ResizeWindow(int width, int height)
{
  vtkInternals* impl = this->Internals;
  /*
   * Here specify the attributes of the desired configuration.
   * Below, we select an EGLConfig with at least 8 bits per color
   * component compatible with on-screen windows
   */
  EGLint surfaceType, clientAPI;
  const EGLint* contextAttribs;
  if (this->OffScreenRendering)
  {
    surfaceType = EGL_PBUFFER_BIT;
    clientAPI = EGL_OPENGL_BIT;
    contextAttribs = NULL;
  }
  else
  {
    surfaceType = EGL_WINDOW_BIT;
    clientAPI = EGL_OPENGL_ES2_BIT;
    const EGLint contextES2[] =
      {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
      };
    contextAttribs = contextES2;
  }
  const EGLint configs[] = {
    EGL_SURFACE_TYPE, surfaceType,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, clientAPI,
    EGL_NONE
  };

  const EGLint surface_attribs[] = {
    EGL_WIDTH, width,
    EGL_HEIGHT, height,
    EGL_NONE
  };


  EGLint numConfigs = 0;
  EGLConfig config;

  if (impl->Display == EGL_NO_DISPLAY)
  {
      if (this->DeviceIndex > 0)
      {
        this->SetDeviceAsDisplay(this->DeviceIndex);
      }
      if (impl->Display == EGL_NO_DISPLAY)
      {
        impl->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      }
    EGLint major = 0, minor = 0;
    eglInitialize(impl->Display, &major, &minor);
    if (this->OffScreenRendering)
    {
      if (major <= 1 && minor < 4)
      {
        vtkErrorMacro("Only EGL 1.4 and greater allows OpenGL as client API. "
                      "See eglBindAPI for more information.");
        return;
      }
      eglBindAPI(EGL_OPENGL_API);
    }
  }


  /* Here, the application chooses the configuration it desires. In this
   * sample, we have a very simplified selection process, where we pick
   * the first EGLConfig that matches our criteria */
  eglChooseConfig(impl->Display, configs, &config, 1, &numConfigs);
  if (numConfigs == 0)
  {
      vtkErrorMacro("No matching EGL configuration found.");
      return;
  }

#ifdef ANDROID
  EGLint format = 0;
  /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
   * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
   * As soon as we picked a EGLConfig, we can safely reconfigure the
   * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
  eglGetConfigAttrib(impl->Display, config, EGL_NATIVE_VISUAL_ID, &format);

  ANativeWindow_setBuffersGeometry(impl->Window, 0, 0, format);
#endif


  if (impl->Context == EGL_NO_CONTEXT)
  {
    impl->Context = eglCreateContext(impl->Display, config, EGL_NO_CONTEXT, contextAttribs);
  }

  if (impl->Surface != EGL_NO_SURFACE)
  {
      eglDestroySurface(impl->Display, impl->Surface);
  }
  impl->Surface = this->OffScreenRendering ?
    eglCreatePbufferSurface(impl->Display, config, surface_attribs):
    eglCreateWindowSurface(impl->Display, config, impl->Window, NULL);
  this->Mapped = 1;
  this->OwnWindow = 1;

  this->MakeCurrent();

  EGLint w, h;
  eglQuerySurface(impl->Display, impl->Surface, EGL_WIDTH, &w);
  eglQuerySurface(impl->Display, impl->Surface, EGL_HEIGHT, &h);
  this->Size[0] = w;
  this->Size[1] = h;
}

void vtkEGLRenderWindow::DestroyWindow()
{
  vtkInternals* impl = this->Internals;
  this->ReleaseGraphicsResources(this);
  if (this->OwnWindow && this->Mapped && impl->Display != EGL_NO_DISPLAY)
  {
    // make sure all other code knows we're not mapped anymore
    this->Mapped = 0;
    eglMakeCurrent(impl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (impl->Context != EGL_NO_CONTEXT)
    {
      eglDestroyContext(impl->Display, impl->Context);
      impl->Context = EGL_NO_CONTEXT;
    }
    if (impl->Surface != EGL_NO_SURFACE)
    {
      eglDestroySurface(impl->Display, impl->Surface);
      impl->Surface = EGL_NO_SURFACE;
    }
    eglTerminate(impl->Display);
    impl->Display = EGL_NO_DISPLAY;
  }
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

  // for offscreen EGL always turn on point sprites
#ifdef VTK_USE_OFFSCREEN_EGL
  glEnable(GL_POINT_SPRITE);
#endif
}

// Initialize the rendering window.
void vtkEGLRenderWindow::Initialize (void)
{
  vtkInternals* impl = this->Internals;
  if (impl->Context == EGL_NO_CONTEXT)
  {
    this->WindowInitialize();
  }
  else
  {
    int w, h;
    this->GetEGLSurfaceSize(&w, &h);
    if (w != this->Size[0] || h != this->Size[1])
    {
      this->ResizeWindow(this->Size[0], this->Size[1]);
    }
  }
  this->Initialized = true;
}

void vtkEGLRenderWindow::Finalize (void)
{
  // clean and destroy window
  this->DestroyWindow();
}

// Change the window to fill the entire screen.
void vtkEGLRenderWindow::SetFullScreen(int vtkNotUsed(arg))
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
void vtkEGLRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width)||(this->Size[1] != height))
  {
    this->Size[0] = width;
    this->Size[1] = height;
    this->Modified();
  }
}

void vtkEGLRenderWindow::GetEGLSurfaceSize(int* width, int* height)
{
  vtkInternals* impl = this->Internals;
  if(impl->Display != EGL_NO_DISPLAY && impl->Surface != EGL_NO_SURFACE)
  {
    EGLint w, h;
    eglQuerySurface(impl->Display, impl->Surface, EGL_WIDTH, &w);
    eglQuerySurface(impl->Display, impl->Surface, EGL_HEIGHT, &h);
    *width = w;
    *height = h;
  }
  else
  {
      *width = 0;
      *height = 0;
  }
}




void vtkEGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInternals* impl = this->Internals;
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << impl->Context << "\n";
  os << indent << "Display: " << impl->Display << "\n";
  os << indent << "Surface: " << impl->Surface << "\n";
}

void vtkEGLRenderWindow::MakeCurrent()
{
  vtkInternals* impl = this->Internals;
  if(this->Mapped &&
     impl->Display != EGL_NO_DISPLAY &&
     impl->Context != EGL_NO_CONTEXT &&
     impl->Surface != EGL_NO_SURFACE)
  {
    if (eglMakeCurrent(impl->Display, impl->Surface, impl->Surface, impl->Context) == EGL_FALSE)
    {
      vtkWarningMacro("Unable to eglMakeCurrent: " << eglGetError());
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
  if ((this->Position[0] != x)||(this->Position[1] != y))
  {
    this->Modified();
  }
  this->Position[0] = x;
  this->Position[1] = y;
}

// Set this RenderWindow to a pre-existing window.
void vtkEGLRenderWindow::SetWindowInfo(char *)
{
  this->OwnWindow = 0;
  this->Mapped = 1;
}

void vtkEGLRenderWindow::SetWindowName(const char *name)
{
  vtkOpenGLRenderWindow::SetWindowName( name );
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

//----------------------------------------------------------------------------
void *vtkEGLRenderWindow::GetGenericDisplayId()
{
  vtkInternals* impl = this->Internals;
  return impl->Display;
}

//----------------------------------------------------------------------------
void* vtkEGLRenderWindow::GetGenericContext()
{
  vtkInternals* impl = this->Internals;
  return impl->Context;
}

//----------------------------------------------------------------------------
void vtkEGLRenderWindow::SetOffScreenRendering (int)
{
  // this is determined at compile time: ANDROID -> 0, VTK_USE_OFFSCREEN_EGL -> 1
}

//----------------------------------------------------------------------------
int vtkEGLRenderWindow::GetOffScreenRendering ()
{
  return this->OffScreenRendering;
}

//----------------------------------------------------------------------------
bool vtkEGLRenderWindow::IsPointSpriteBugPresent()
{
  // eventually we'll want to check with the NVIDIA EGL version to see if the
  // point sprite bug is fixed but we don't know yet when it will be fixed
  // but we do know that it's present in both the 355 and 358 drivers. for
  // now do the safe thing and just assume the bug isn't fixed until we
  // find a driver version where it is fixed.
  this->IsPointSpriteBugTested = true;
  this->IsPointSpriteBugPresent_ = true;
  // if (! this->IsPointSpriteBugTested)
  //   {
  //   this->IsPointSpriteBugTested = true;
  //   this->IsPointSpriteBugPresent_ =
  //     (strcmp(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "4.5.0 NVIDIA 355.11") == 0) ||
  //     (strcmp(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "4.5.0 NVIDIA 358.16") == 0);
  //   }
  return this->IsPointSpriteBugPresent_;
}

//----------------------------------------------------------------------------
void vtkEGLRenderWindow::SetWindowId(void *window)
{
  vtkInternals* impl = this->Internals;
  impl->Window = reinterpret_cast<EGLNativeWindowType>(window);
}
