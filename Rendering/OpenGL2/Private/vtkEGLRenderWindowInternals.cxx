// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This must be included first because of conflicts between fmt and egl.h on Windows
#include "vtkStringScanner.h"

#include "Private/vtkEGLRenderWindowInternals.h"

#include "vtkLogger.h"

#include <cassert>

#if defined(__ANDROID__) || defined(ANDROID)
#include "Private/vtkEGLAndroidConfig.h"
#elif defined(USE_WAYLAND)
#include "Private/vtkEGLWaylandConfig.h"
#else
#include "Private/vtkEGLDefaultConfig.h"
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
struct vtkEGLDisplayInitializationHelper
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

//------------------------------------------------------------------------------
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
  bool IsAvailable() { return this->Available; }
  EGLQueryDevicesType eglQueryDevices;
  EGLGetPlatformDisplayType eglGetPlatformDisplay;

private:
  vtkEGLDeviceExtensions()
  {
    this->Available = false;
    this->eglQueryDevices = nullptr;
    this->eglGetPlatformDisplay = nullptr;
    const char* availableProperties = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (availableProperties == nullptr)
    {
      // eglQueryString returns a nullptr upon failure.
      // Setting it to empty string to silently ignore failure.
      availableProperties = "";
    }
    std::string platformExtensions(availableProperties);
    if (platformExtensions.find("EGL_EXT_device_base") != std::string::npos &&
      platformExtensions.find("EGL_EXT_platform_device") != std::string::npos &&
      platformExtensions.find("EGL_EXT_platform_base") != std::string::npos)
    {
      this->eglQueryDevices = (EGLQueryDevicesType)eglGetProcAddress("eglQueryDevicesEXT");
      this->eglGetPlatformDisplay =
        (EGLGetPlatformDisplayType)eglGetProcAddress("eglGetPlatformDisplayEXT");
      if (this->eglQueryDevices && this->eglGetPlatformDisplay)
      {
        this->Available = true;
      }
    }
  }

  bool Available;
};
}

//------------------------------------------------------------------------------
vtkEGLRenderWindowInternals::vtkEGLRenderWindowInternals()
  : Window((EGLNativeWindowType) nullptr)
  , Display(EGL_NO_DISPLAY)
  , Surface(EGL_NO_SURFACE)
  , Context(EGL_NO_CONTEXT)
{
#if defined(__ANDROID__) || defined(ANDROID)
  this->Config = std::make_unique<vtkEGLAndroidConfig>();
#elif defined(USE_WAYLAND)
  this->Config = std::make_unique<vtkEGLWaylandConfig>();
#else
  this->Config = std::make_unique<vtkEGLDefaultConfig>();
#endif

  gladLoaderLoadEGL(this->Display);

  // Use an environment variable to set the default device index
  char* EGLDefaultDeviceIndexEnv = std::getenv("VTK_DEFAULT_EGL_DEVICE_INDEX");
  if (EGLDefaultDeviceIndexEnv)
  {
    VTK_FROM_CHARS_IF_ERROR_RETURN(
      EGLDefaultDeviceIndexEnv, vtkEGLDisplayInitializationHelper::DefaultDeviceIndex, );
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::DestroyWindow()
{
  if (this->Display != EGL_NO_DISPLAY)
  {
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
    vtkEGLDisplayInitializationHelper::Terminate(this->Display);
    this->Display = EGL_NO_DISPLAY;
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::SwapBuffer()
{
  if (this->Display != EGL_NO_DISPLAY)
  {
    eglSwapBuffers(this->Display, this->Surface);
  }
  else
  {
    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
  }
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindowInternals::SetDeviceAsDisplay(int deviceIndex)
{
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  bool foundWorkingDisplay = false;
  EGLint major = 0, minor = 0;

  if (ext->IsAvailable())
  {
    EGLint num_devices = 0;
    ext->eglQueryDevices(num_devices, nullptr, &num_devices);
    if (deviceIndex >= num_devices)
    {
      vtkLog(WARNING,
        "EGL device index: " << deviceIndex
                             << " is greater than "
                                "the number of supported deviced in the system: "
                             << num_devices);
    }

    std::vector<EGLDeviceEXT> devices(num_devices);
    ext->eglQueryDevices(num_devices, devices.data(), &num_devices);

    if (deviceIndex >= 0)
    {
      this->Display =
        ext->eglGetPlatformDisplay(this->Config->GetPlatform(), devices[deviceIndex], nullptr);

      if (vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor) == EGL_FALSE)
      {
        vtkLog(WARNING, "EGL device index: " << deviceIndex << " could not be initialized.");
      }

      foundWorkingDisplay = true;
    }
    else
    {
      EGLDisplay extDisplay = this->Config->GetDisplay();
      if (extDisplay == EGL_NO_DISPLAY)
      {
        extDisplay = devices[vtkEGLDisplayInitializationHelper::DefaultDeviceIndex];
      }

      this->Display = ext->eglGetPlatformDisplay(this->Config->GetPlatform(), extDisplay, nullptr);

      if (vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor) == EGL_FALSE)
      {
        vtkLog(WARNING,
          "EGL device index: " << vtkEGLDisplayInitializationHelper::DefaultDeviceIndex
                               << " could not be initialized. Trying other devices...");

        for (int i = 0; i < num_devices; i++)
        {
          // Don't check DefaultDeviceIndex again
          if (i == vtkEGLDisplayInitializationHelper::DefaultDeviceIndex)
          {
            continue;
          }

          this->Display =
            ext->eglGetPlatformDisplay(this->Config->GetPlatform(), devices[i], nullptr);
          if (vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor) ==
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
    vtkLog(WARNING,
      "Setting an EGL display to device index: "
        << deviceIndex
        << " require "
           "EGL_EXT_device_base EGL_EXT_platform_device EGL_EXT_platform_base extensions");

    vtkLog(WARNING, "Attempting to use the default egl display for the current platform...");
    this->Display =
      eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(this->Config->GetDisplay()));

    if (vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor) == EGL_FALSE)
    {
      vtkLog(WARNING, "Could not initialize a device. Exiting...");
      return false;
    }
  }

#if !defined(__ANDROID__) && !defined(ANDROID)
  if (major <= 1 && minor < 4)
  {
    vtkLog(WARNING,
      "Only EGL 1.4 and greater allows OpenGL as client API. "
      "See eglBindAPI for more information.");
    return false;
  }
  // Loads EGL functions that are supported by this display.
  gladLoaderLoadEGL(this->Display);
  eglBindAPI(EGL_OPENGL_API);
#endif

  return true;
}

//------------------------------------------------------------------------------
int vtkEGLRenderWindowInternals::GetNumberOfDevices()
{
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  if (ext->IsAvailable())
  {
    EGLint num_devices = 0;
    ext->eglQueryDevices(num_devices, nullptr, &num_devices);
    return num_devices;
  }
  vtkLog(WARNING,
    "Getting the number of devices (graphics cards) on a system require "
    "EGL_EXT_device_base, EGL_EXT_platform_device and EGL_EXT_platform_base extensions");
  return 0;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::ConfigureWindow(int width, int height)
{
  if (this->Display == EGL_NO_DISPLAY)
  {
    if (!this->SetDeviceAsDisplay(this->DeviceIndex))
    {
      vtkLog(WARNING,
        "Could not set device as display. "
        "EGL_EXT_device_base, EGL_EXT_platform_device and EGL_EXT_platform_base extensions are "
        "required.");
      return;
    }
  }

  EGLint surfaceType, clientAPI;
  if (this->UseOnscreenRendering)
  {
    surfaceType = EGL_WINDOW_BIT;
    clientAPI = EGL_OPENGL_ES2_BIT;
  }
  else
  {
    surfaceType = EGL_PBUFFER_BIT;
    clientAPI = EGL_OPENGL_BIT;
  }

  const EGLint configs[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_DEPTH_SIZE,
    24, EGL_SURFACE_TYPE, surfaceType, EGL_RENDERABLE_TYPE, clientAPI, EGL_NONE };
  EGLint numConfigs = 0;
  EGLConfig config;

  if (eglInitialize(this->Display, nullptr, nullptr) == EGL_FALSE)
  {
    vtkLog(WARNING, "EGL initialization failed.");
    return;
  }

  eglChooseConfig(this->Display, configs, &config, 1, &numConfigs);
  if (numConfigs == 0)
  {
    vtkLog(WARNING, "No matching EGL configurations found. ");
    return;
  }

  if (this->Context == EGL_NO_CONTEXT)
  {
    this->Config->CreateContext(this->Context, this->Display, config);
    if (this->Context == EGL_NO_CONTEXT)
    {
      vtkLog(WARNING, "Failed to create EGL context.");
      return;
    }
  }

  if (this->Surface != EGL_NO_SURFACE)
  {
    eglDestroySurface(this->Display, this->Surface);
    this->Surface = EGL_NO_SURFACE;
  }

  this->Config->CreateWindowSurface(this->Surface, this->Display, config, width, height);
  if (this->Surface == EGL_NO_SURFACE)
  {
    vtkLog(WARNING, "Failed to create EGL window surface.");
    return;
  }

  this->MakeCurrent();
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::GetSizeFromSurface(int* size)
{
  EGLint w, h;
  eglQuerySurface(this->Display, this->Surface, EGL_WIDTH, &w);
  eglQuerySurface(this->Display, this->Surface, EGL_HEIGHT, &h);
  size[0] = w;
  size[1] = h;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::ReleaseCurrent()
{
  if (this->Display != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(this->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindowInternals::MakeCurrent()
{
  bool hasFailed =
    eglMakeCurrent(this->Display, this->Surface, this->Surface, this->Context) == EGL_FALSE;

  return hasFailed;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::SetUseOnscreenRendering(bool useOnscreenRendering)
{
  this->UseOnscreenRendering = useOnscreenRendering;
  if (this->Config)
  {
    this->Config->SetOnscreenRendering(useOnscreenRendering);
  }
}
VTK_ABI_NAMESPACE_END
