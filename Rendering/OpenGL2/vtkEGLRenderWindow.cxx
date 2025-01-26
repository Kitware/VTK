// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEGLRenderWindow.h"

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkType.h"
#include "vtksys/SystemTools.hxx"

#include "vtk_glad.h"
#include "vtkglad/include/glad/egl.h"

#include <atomic>
#include <cassert>
#include <sstream>

#if defined(__ANDROID__) || defined(ANDROID)
#include <android/native_window.h>
#include <vtkAndroidRenderWindowInteractor.h>
#endif

VTK_ABI_NAMESPACE_BEGIN
namespace
{
typedef void* EGLDeviceEXT;
typedef EGLBoolean (*EGLQueryDevicesType)(EGLint, EGLDeviceEXT*, EGLint*);
typedef EGLDisplay (*EGLGetPlatformDisplayType)(EGLenum, void*, const EGLint*);
/**
 * EGLDisplay provided by eglGetDisplay() call can be same handle for multiple
 * instances of vtkEGLRenderWindow. In which case, while it's safe to call
 * eglInitialize() repeatedly, eglTerminate() should only be called once after
 * the final instance of the window is destroyed. This class helps us do
 * that. See paraview/paraview#16928.
 */
class vtkEGLDisplayInitializationHelper
{
  static std::map<EGLDisplay, std::atomic<int64_t>> DisplayUsageCounts;

public:
  static EGLBoolean Initialize(EGLDisplay dpy, EGLint* major, EGLint* minor)
  {
    ++DisplayUsageCounts[dpy];
    return eglInitialize(dpy, major, minor);
  }
  static EGLBoolean Terminate(EGLDisplay dpy)
  {
    assert(DisplayUsageCounts.find(dpy) != DisplayUsageCounts.end());
    if (--DisplayUsageCounts[dpy] == 0)
    {
      DisplayUsageCounts.erase(dpy);
      return eglTerminate(dpy);
    }
    return EGL_TRUE;
  }

  static int DefaultDeviceIndex;
};

std::map<EGLDisplay, std::atomic<int64_t>> vtkEGLDisplayInitializationHelper::DisplayUsageCounts;
int vtkEGLDisplayInitializationHelper::DefaultDeviceIndex = VTK_DEFAULT_EGL_DEVICE_INDEX;

struct vtkEGLDeviceExtensions
{
  static vtkEGLDeviceExtensions* GetInstance()
  {
    static vtkEGLDeviceExtensions* instance = nullptr;
    if (instance == nullptr)
    {
      instance = new vtkEGLDeviceExtensions();
    }
    return instance;
  }
  bool Available() { return this->Available_; }
  bool Available_;
  EGLQueryDevicesType eglQueryDevices;
  EGLGetPlatformDisplayType eglGetPlatformDisplay;

private:
  vtkEGLDeviceExtensions()
  {
    this->Available_ = false;
    this->eglQueryDevices = nullptr;
    this->eglGetPlatformDisplay = nullptr;
    const char* s = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (s == nullptr)
    {
      // eglQueryString returns a nullptr upon failure.
      // Setting it to empty string to silently ignore failure.
      s = "";
    }
    std::string platformExtensions(s);
    if (platformExtensions.find("EGL_EXT_device_base") != std::string::npos &&
      platformExtensions.find("EGL_EXT_platform_device") != std::string::npos &&
      platformExtensions.find("EGL_EXT_platform_base") != std::string::npos)
    {
      this->eglQueryDevices = (EGLQueryDevicesType)eglGetProcAddress("eglQueryDevicesEXT");
      this->eglGetPlatformDisplay =
        (EGLGetPlatformDisplayType)eglGetProcAddress("eglGetPlatformDisplayEXT");
      if (this->eglQueryDevices && this->eglGetPlatformDisplay)
      {
        this->Available_ = true;
      }
    }
  }
};
};

vtkStandardNewMacro(vtkEGLRenderWindow);

class vtkEGLRenderWindow::vtkInternals
{
public:
  EGLNativeWindowType Window;
  EGLDisplay Display;
  EGLSurface Surface;
  EGLContext Context;
  bool EGLInitialized; // When set, it means methods from EGL 1.4 and higher are accessible.
  vtkInternals()
    : Window((EGLNativeWindowType)0)
    , Display(EGL_NO_DISPLAY)
    , Surface(EGL_NO_SURFACE)
    , Context(EGL_NO_CONTEXT)
    , EGLInitialized(false)
  {
#if defined(__ANDROID__) || defined(ANDROID)
    // On Android platform, unconditionally load the EGL functions with EGL_NO_DISPLAY
    // because the EGL library is always available.
    gladLoaderLoadEGL(this->Display);
    this->EGLInitialized = true;
#endif
  }
};

vtkEGLRenderWindow::vtkEGLRenderWindow()
{
  this->Internals = new vtkInternals();
  this->OwnWindow = true;
  this->ScreenSize[0] = 1920;
  this->ScreenSize[1] = 1080;

  // this is initialized in vtkRenderWindow
  // so we don't need to initialize on else
  this->DeviceIndex = -1;
#if !defined(__ANDROID__) && !defined(ANDROID)
  this->ShowWindow = false;
#endif

  // Use an environment variable to set the default device index
  char* EGLDefaultDeviceIndexEnv = std::getenv("VTK_DEFAULT_EGL_DEVICE_INDEX");
  if (EGLDefaultDeviceIndexEnv)
  {
    try
    {
      int index = atoi(EGLDefaultDeviceIndexEnv);
      if (index >= 0)
      {
        vtkEGLDisplayInitializationHelper::DefaultDeviceIndex = index;
      }
    }
    catch (const std::out_of_range&)
    {
    }
    catch (const std::invalid_argument&)
    {
    }
  }

  // If the device index is explicitly set then we need to use it.
  char* EGLDeviceIndexEnv = std::getenv("VTK_EGL_DEVICE_INDEX");
  if (EGLDeviceIndexEnv)
  {
    try
    {
      int index = atoi(EGLDeviceIndexEnv);
      if (index >= 0)
      {
        this->DeviceIndex = index;
      }
    }
    catch (const std::out_of_range&)
    {
    }
    catch (const std::invalid_argument&)
    {
    }
  }

  this->IsPointSpriteBugTested = false;
  this->IsPointSpriteBugPresent_ = false;

  auto loadFunc = [](void*, const char* name) -> VTKOpenGLAPIProc
  {
    if (name)
    {
      return eglGetProcAddress(name);
    }
    return nullptr;
  };
  this->SetOpenGLSymbolLoader(loadFunc, nullptr);
}

// free up memory & close the window
vtkEGLRenderWindow::~vtkEGLRenderWindow()
{
  // close-down all system-specific drawing resources
  this->Finalize();

  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }
  delete this->Internals;
}

// End the rendering process and display the image.
void vtkEGLRenderWindow::Frame()
{
  vtkInternals* impl = this->Internals;
  this->MakeCurrent();
  this->Superclass::Frame();
  if (this->OwnWindow)
  {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers &&
      impl->Display != EGL_NO_DISPLAY)
    {
      eglSwapBuffers(impl->Display, impl->Surface);
      glFinish();
      vtkDebugMacro(<< " eglSwapBuffers\n");
    }
  }
  else
  {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
    {
      eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
      glFinish();
      vtkDebugMacro(<< " eglSwapBuffers\n");
    }
  }
}

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkEGLRenderWindow::SetStereoCapableWindow(vtkTypeBool capable)
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

// Specify the size of the rendering window.
void vtkEGLRenderWindow::SetSize(int width, int height)
{
  this->Superclass::SetSize(width, height);
  vtkInternals* impl = this->Internals;

  if (this->OwnWindow && impl->Display != EGL_NO_DISPLAY && impl->Surface != EGL_NO_SURFACE)
  {
    // We only need to resize the window if we own it
    int w, h;
    this->GetEGLSurfaceSize(&w, &h);
    if (w != this->Size[0] || h != this->Size[1])
    {
      this->ResizeWindow(this->Size[0], this->Size[1]);
    }
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
    ext->eglQueryDevices(num_devices, nullptr, &num_devices);
    return num_devices;
  }
  vtkWarningMacro(
    "Getting the number of devices (graphics cards) on a system require "
    "EGL_EXT_device_base, EGL_EXT_platform_device and EGL_EXT_platform_base extensions");
  return 0;
}

bool vtkEGLRenderWindow::SetDeviceAsDisplay(int deviceIndex)
{
  vtkInternals* impl = this->Internals;
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  bool foundWorkingDisplay = false;
  EGLint major = 0, minor = 0;

  if (ext->Available())
  {
    EGLint num_devices = 0;
    ext->eglQueryDevices(num_devices, nullptr, &num_devices);
    if (deviceIndex >= num_devices)
    {
      vtkWarningMacro("EGL device index: " << deviceIndex
                                           << " is greater than "
                                              "the number of supported deviced in the system: "
                                           << num_devices);
    }

    std::vector<EGLDeviceEXT> devices(num_devices);
    ext->eglQueryDevices(num_devices, devices.data(), &num_devices);

    if (deviceIndex >= 0)
    {
      impl->Display =
        ext->eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[deviceIndex], nullptr);

      if (vtkEGLDisplayInitializationHelper::Initialize(impl->Display, &major, &minor) == EGL_FALSE)
      {
        vtkWarningMacro(
          "EGL device index: " << deviceIndex << " could not be initialized. Exiting...");
      }

      foundWorkingDisplay = true;
    }
    else
    {
      impl->Display = ext->eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT,
        devices[vtkEGLDisplayInitializationHelper::DefaultDeviceIndex], nullptr);

      if (vtkEGLDisplayInitializationHelper::Initialize(impl->Display, &major, &minor) == EGL_FALSE)
      {
        vtkWarningMacro(
          "EGL device index: " << vtkEGLDisplayInitializationHelper::DefaultDeviceIndex
                               << " could not be initialized. Trying other devices...");

        for (int i = 0; i < num_devices; i++)
        {
          // Don't check DefaultDeviceIndex again
          if (i == vtkEGLDisplayInitializationHelper::DefaultDeviceIndex)
          {
            continue;
          }

          impl->Display = ext->eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);
          if (vtkEGLDisplayInitializationHelper::Initialize(impl->Display, &major, &minor) ==
            EGL_TRUE)
          {
            foundWorkingDisplay = true;
            break;
          }
        }
      }
      else
      {
        foundWorkingDisplay = true;
      }
    }
  }

  if (!foundWorkingDisplay)
  {
    // eglGetDisplay(EGL_DEFAULT_DISPLAY) does not seem to work
    // if there are several cards on a system.
    vtkWarningMacro("Setting an EGL display to device index: "
      << deviceIndex
      << " require "
         "EGL_EXT_device_base EGL_EXT_platform_device EGL_EXT_platform_base extensions");

    vtkWarningMacro("Attempting to use EGL_DEFAULT_DISPLAY...");
    impl->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (vtkEGLDisplayInitializationHelper::Initialize(impl->Display, &major, &minor) == EGL_FALSE)
    {
      vtkWarningMacro("Could not initialize a device. Exiting...");
      return false;
    }
  }
#if !defined(__ANDROID__) && !defined(ANDROID)
  if (major <= 1 && minor < 4)
  {
    vtkWarningMacro("Only EGL 1.4 and greater allows OpenGL as client API. "
                    "See eglBindAPI for more information.");
    return false;
  }
  // Loads EGL functions that are supported by this display.
  gladLoaderLoadEGL(impl->Display);
  impl->EGLInitialized = true;
  eglBindAPI(EGL_OPENGL_API);
#endif

  return true;
}

void vtkEGLRenderWindow::SetShowWindow(bool val)
{
#if !defined(__ANDROID__) && !defined(ANDROID)
  if (val)
  {
    vtkWarningMacro("vtkEGLRenderWindow does not support showing the window onscreen");
  }
#endif
  this->Superclass::SetShowWindow(val);
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
#if defined(__ANDROID__) || defined(ANDROID)
  surfaceType = EGL_WINDOW_BIT;
  clientAPI = EGL_OPENGL_ES2_BIT;
  const EGLint contextES2[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  contextAttribs = contextES2;
#else
  // arguably you could have EGL_WINDOW_BIT here as well
  surfaceType = EGL_PBUFFER_BIT;
  clientAPI = EGL_OPENGL_BIT;
  contextAttribs = nullptr;
#endif

  const EGLint configs[] = { EGL_SURFACE_TYPE, surfaceType, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 8, EGL_RENDERABLE_TYPE, clientAPI,
    EGL_NONE };

#if !defined(__ANDROID__) && !defined(ANDROID)
  const EGLint surface_attribs[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
#endif

  EGLint numConfigs = 0;
  EGLConfig config;

  if (impl->Display == EGL_NO_DISPLAY)
  {
    if (!this->SetDeviceAsDisplay(this->DeviceIndex))
    {
      return;
    }
  }

  /* Here, the application chooses the configuration it desires. In this
   * sample, we have a very simplified selection process, where we pick
   * the first EGLConfig that matches our criteria */
  eglChooseConfig(impl->Display, configs, &config, 1, &numConfigs);
  if (numConfigs == 0)
  {
    vtkWarningMacro("No matching EGL configuration found.");
    return;
  }

#if defined(__ANDROID__) || defined(ANDROID)
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

#if defined(__ANDROID__) || defined(ANDROID)
  impl->Surface = eglCreateWindowSurface(impl->Display, config, impl->Window, nullptr);
#else
  impl->Surface = eglCreatePbufferSurface(impl->Display, config, surface_attribs);
#endif
  this->Mapped = this->ShowWindow;
  this->OwnWindow = true;

#if defined(__ANDROID__) || defined(ANDROID)
  vtkAndroidRenderWindowInteractor* interactor =
    vtkAndroidRenderWindowInteractor::SafeDownCast(this->Interactor);
  if (interactor)
  {
    interactor->SetOwnWindow(this->OwnWindow);
  }
#endif

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
  if (this->OwnWindow && impl->Display != EGL_NO_DISPLAY)
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
    vtkEGLDisplayInitializationHelper::Terminate(impl->Display);
    impl->Display = EGL_NO_DISPLAY;
    impl->EGLInitialized = false;
  }
}

// Initialize the window for rendering.
void vtkEGLRenderWindow::WindowInitialize()
{
  vtkInternals* impl = this->Internals;
  if (this->OwnWindow)
  {
    this->CreateAWindow();
  }
  else if (impl->Context == EGL_NO_CONTEXT)
  {
    // Get our current context from the EGL current context
    impl->Context = eglGetCurrentContext();
  }

  this->MakeCurrent();

  // tell our renderers about us
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
  {
    ren->SetRenderWindow(nullptr);
    ren->SetRenderWindow(this);
  }

  this->OpenGLInit();

  // for offscreen EGL always turn on point sprites
#if !defined(__ANDROID__) && !defined(ANDROID) && defined(GL_POINT_SPRITE)
  if (this->Initialized)
  {
    glEnable(GL_POINT_SPRITE);
  }
#endif

#if defined(__ANDROID__) || defined(ANDROID)
  vtkAndroidRenderWindowInteractor* interactor =
    vtkAndroidRenderWindowInteractor::SafeDownCast(this->Interactor);
  if (interactor)
  {
    interactor->SetOwnWindow(this->OwnWindow);
  }
#endif
}

// Initialize the rendering window.
void vtkEGLRenderWindow::Initialize()
{
  vtkInternals* impl = this->Internals;
  if (impl->Context == EGL_NO_CONTEXT)
  {
    this->WindowInitialize();
  }
}

void vtkEGLRenderWindow::Finalize()
{
  // clean and destroy window
  this->DestroyWindow();
}

// Change the window to fill the entire screen.
void vtkEGLRenderWindow::SetFullScreen(vtkTypeBool vtkNotUsed(arg))
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

void vtkEGLRenderWindow::GetEGLSurfaceSize(int* width, int* height)
{
  vtkInternals* impl = this->Internals;
  if (impl->Display != EGL_NO_DISPLAY && impl->Surface != EGL_NO_SURFACE)
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
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Context: " << impl->Context << "\n";
  os << indent << "Display: " << impl->Display << "\n";
  os << indent << "Surface: " << impl->Surface << "\n";
}

void vtkEGLRenderWindow::MakeCurrent()
{
  vtkInternals* impl = this->Internals;
  if (impl->Display != EGL_NO_DISPLAY && impl->Context != EGL_NO_CONTEXT &&
    impl->Surface != EGL_NO_SURFACE)
  {
    if (eglMakeCurrent(impl->Display, impl->Surface, impl->Surface, impl->Context) == EGL_FALSE)
    {
      vtkWarningMacro("Unable to eglMakeCurrent: " << eglGetError());
      return;
    }
  }
}

void vtkEGLRenderWindow::ReleaseCurrent()
{
  vtkInternals* impl = this->Internals;
  if (impl->Display != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(impl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
}

//------------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkEGLRenderWindow::IsCurrent()
{
  return true;
}

// Get the size of the screen in pixels
int* vtkEGLRenderWindow::GetScreenSize()
{
  // TODO: actually determine screensize.

  return this->ScreenSize;
}

// Get the position in screen coordinates (pixels) of the window.
int* vtkEGLRenderWindow::GetPosition()
{
  return this->Position;
}

// Move the window to a new position on the display.
void vtkEGLRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
  }
  this->Position[0] = x;
  this->Position[1] = y;
}

// Set this RenderWindow to a pre-existing window.
void vtkEGLRenderWindow::SetWindowInfo(const char*)
{
  this->Mapped = 1;
  this->OwnWindow = false;

#if defined(__ANDROID__) || defined(ANDROID)
  vtkAndroidRenderWindowInteractor* interactor =
    vtkAndroidRenderWindowInteractor::SafeDownCast(this->Interactor);
  if (interactor)
  {
    interactor->SetOwnWindow(this->OwnWindow);
  }
#endif
}

void vtkEGLRenderWindow::SetWindowName(const char* name)
{
  vtkOpenGLRenderWindow::SetWindowName(name);
}

void vtkEGLRenderWindow::Render()
{
  // Now do the superclass stuff
  this->vtkOpenGLRenderWindow::Render();
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::HideCursor() {}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::ShowCursor() {}

//------------------------------------------------------------------------------
void* vtkEGLRenderWindow::GetGenericDisplayId()
{
  vtkInternals* impl = this->Internals;
  return impl->Display;
}

//------------------------------------------------------------------------------
void* vtkEGLRenderWindow::GetGenericContext()
{
  vtkInternals* impl = this->Internals;
  return impl->Context;
}

//------------------------------------------------------------------------------
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
  //     (strcmp(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "4.5.0 NVIDIA 355.11") ==
  //     0) || (strcmp(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "4.5.0 NVIDIA
  //     358.16") == 0);
  //   }
  return this->IsPointSpriteBugPresent_;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetWindowId(void* window)
{
  vtkInternals* impl = this->Internals;
  impl->Window = reinterpret_cast<EGLNativeWindowType>(window);
}

//------------------------------------------------------------------------------
const char* vtkEGLRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  auto& internals = (*this->Internals);
  if (!internals.Display)
  {
    return "Display ID not set";
  }
  const char* eglVersion = eglQueryString(internals.Display, EGL_VERSION);
  const char* eglVendor = eglQueryString(internals.Display, EGL_VENDOR);
  const char* eglClientAPIs = eglQueryString(internals.Display, EGL_CLIENT_APIS);
  const char* eglExtensions = eglQueryString(internals.Display, EGL_EXTENSIONS);
  const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
  const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));

  std::ostringstream strm;
  strm << "EGL version string:  " << eglVersion << endl;
  strm << "EGL vendor string:  " << eglVendor << endl;
  strm << "EGL client APIs:  " << eglClientAPIs << endl;
  strm << "EGL extensions:  " << eglExtensions << endl;
  strm << "OpenGL vendor string:  " << glVendor << endl;
  strm << "OpenGL renderer string:  " << glRenderer << endl;
  strm << "OpenGL version string:  " << glVersion << endl;
  strm << "OpenGL extensions:  " << endl;
  int n = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  for (int i = 0; i < n; i++)
  {
    const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
    strm << "  " << ext << endl;
  }
  delete[] this->Capabilities;

  const auto capsString = strm.str();
  size_t len = capsString.length();
  this->Capabilities = new char[len + 1];
  strncpy(this->Capabilities, capsString.c_str(), len);
  this->Capabilities[len] = 0;

  return this->Capabilities;
}

VTK_ABI_NAMESPACE_END
