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
#include "Private/vtkEGLMesaConfig.h"
#endif

VTK_ABI_NAMESPACE_BEGIN

namespace
{
typedef void* EGLDeviceEXT;
typedef EGLBoolean (*EGLQueryDevicesType)(EGLint, EGLDeviceEXT*, EGLint*);
typedef EGLDisplay (*EGLGetPlatformDisplayType)(EGLenum, void*, const EGLint*);
typedef EGLDisplay (*EGLGetPlatformDisplayEXTType)(EGLenum, void*, const EGLint*);

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

  EGLQueryDevicesType EglQueryDevices;
  EGLGetPlatformDisplayType EglGetPlatformDisplay;

  typedef const char* (*EGLQueryDeviceStringEXTType)(EGLDeviceEXT, EGLint);
  EGLQueryDeviceStringEXTType EglQueryDeviceStringEXT;

private:
  vtkEGLDeviceExtensions()
  {
    this->Available = false;
    this->EglQueryDevices = nullptr;
    this->EglGetPlatformDisplay = nullptr;
    this->EglQueryDeviceStringEXT = nullptr;

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
      this->EglQueryDevices = (EGLQueryDevicesType)eglGetProcAddress("eglQueryDevicesEXT");
      this->EglGetPlatformDisplay =
        (EGLGetPlatformDisplayType)eglGetProcAddress("eglGetPlatformDisplayEXT");
      this->EglQueryDeviceStringEXT =
        (EGLQueryDeviceStringEXTType)eglGetProcAddress("eglQueryDeviceStringEXT");

      if (this->EglQueryDevices && this->EglGetPlatformDisplay)
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
#elif defined(VTK_USE_MESA_SOFTWARE_RENDERING)
  this->Config = std::make_unique<vtkEGLMesaConfig>();
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
    this->DestroyOffscreenFramebuffer();
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
bool vtkEGLRenderWindowInternals::SetDeviceAsDisplay(int deviceIndex)
{
  bool foundWorkingDisplay = false;
  EGLint major = 0, minor = 0;

  if (this->IsMesaSoftwareRenderer())
  {
    foundWorkingDisplay = this->TryInitializeMesaSoftware(major, minor);
  }
  else
  {
    foundWorkingDisplay = this->TryInitializeHardware(deviceIndex, major, minor);
  }

  if (foundWorkingDisplay)
  {
    return this->FinalizeDisplaySetup(major, minor);
  }

  vtkLog(ERROR, "Failed to initialize any EGL Display");
  return false;
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindowInternals::TryInitializeMesaSoftware(EGLint& major, EGLint& minor)
{
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  if (!ext->IsAvailable() || !ext->EglGetPlatformDisplay)
  {
    return false;
  }

  // 1. Try to find the explicit EGL_MESA_device_software extension
  if (ext->EglQueryDeviceStringEXT)
  {
    EGLint num_devices = 0;
    ext->EglQueryDevices(0, nullptr, &num_devices);

    if (num_devices > 0)
    {
      std::vector<EGLDeviceEXT> devices(num_devices);
      ext->EglQueryDevices(num_devices, devices.data(), &num_devices);

      for (int i = 0; i < num_devices; ++i)
      {
        const char* deviceExts = ext->EglQueryDeviceStringEXT(devices[i], EGL_EXTENSIONS);
        if (deviceExts && strstr(deviceExts, "EGL_MESA_device_software"))
        {
          vtkLog(TRACE, "Found EGL_MESA_device_software at device index " << i);
          this->Display = ext->EglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[i], nullptr);

          if (this->Display != EGL_NO_DISPLAY &&
            vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor))
          {
            vtkLog(TRACE,
              "Initialized Surfaceless EGL " << major << "." << minor << " on Software Device");
            return true;
          }

          vtkLog(ERROR, "Failed to initialize the software device. EGL Error: " << eglGetError());
        }
      }
    }
  }

  // 2. Fallback: Try generic platform surfaceless mesa
  this->Display =
    ext->EglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
  if (this->Display != EGL_NO_DISPLAY &&
    vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor))
  {
    vtkLog(TRACE, "Initialized Default Surfaceless EGL " << major << "." << minor);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindowInternals::TryInitializeHardware(
  int deviceIndex, EGLint& major, EGLint& minor)
{
  vtkEGLDeviceExtensions* ext = vtkEGLDeviceExtensions::GetInstance();
  bool foundWorkingDisplay = false;

  if (!ext->IsAvailable())
  {
    return false;
  }

  EGLint num_devices = 0;
  ext->EglQueryDevices(num_devices, nullptr, &num_devices);
  if (deviceIndex >= num_devices)
  {
    vtkLog(WARNING,
      "EGL device index: " << deviceIndex
                           << " is greater than "
                              "the number of supported deviced in the system: "
                           << num_devices);
  }

  std::vector<EGLDeviceEXT> devices(num_devices);
  ext->EglQueryDevices(num_devices, devices.data(), &num_devices);

  if (deviceIndex >= 0)
  {
    this->Display =
      ext->EglGetPlatformDisplay(this->Config->GetPlatform(), devices[deviceIndex], nullptr);

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

    this->Display = ext->EglGetPlatformDisplay(this->Config->GetPlatform(), extDisplay, nullptr);

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
          ext->EglGetPlatformDisplay(this->Config->GetPlatform(), devices[i], nullptr);
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
    if (vtkEGLDisplayInitializationHelper::Initialize(this->Display, &major, &minor) == EGL_TRUE)
    {
      return true;
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

  return true;
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindowInternals::FinalizeDisplaySetup(EGLint major, EGLint minor)
{
#if !defined(__ANDROID__) && !defined(ANDROID)
  if (major < 1 || (major == 1 && minor < 4))
  {
    vtkLog(ERROR,
      "Only EGL 1.4 and greater allows OpenGL as client API. "
      "See eglBindAPI for more information. (Current: "
        << major << "." << minor << ")");
    return false;
  }
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
    ext->EglQueryDevices(num_devices, nullptr, &num_devices);
    return num_devices;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::ConfigureWindow(int width, int height)
{
  this->Width = width;
  this->Height = height;

  // 1. Ensure Display is initialized (from hardware/Wayland logic)
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

  if (eglInitialize(this->Display, nullptr, nullptr) == EGL_FALSE)
  {
    vtkLog(WARNING, "EGL initialization failed.");
    return;
  }

  // 2. Build EGL configuration dynamically based on the renderer
  std::vector<EGLint> configAttribs = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 24 };

  if (this->IsMesaSoftwareRenderer())
  {
    // Mesa Software Config: Force Desktop GL, Pbuffer, Alpha, and Stencil
    configAttribs.push_back(EGL_ALPHA_SIZE);
    configAttribs.push_back(8);
    configAttribs.push_back(EGL_STENCIL_SIZE);
    configAttribs.push_back(8);
    configAttribs.push_back(EGL_SURFACE_TYPE);
    configAttribs.push_back(EGL_PBUFFER_BIT);
    configAttribs.push_back(EGL_RENDERABLE_TYPE);
    configAttribs.push_back(EGL_OPENGL_BIT);
  }
  else
  {
    // Hardware/Wayland Config: Dynamic based on onscreen vs offscreen
    EGLint surfaceType = this->UseOnscreenRendering ? EGL_WINDOW_BIT : EGL_PBUFFER_BIT;
    EGLint clientAPI = this->UseOnscreenRendering ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_BIT;

    configAttribs.push_back(EGL_SURFACE_TYPE);
    configAttribs.push_back(surfaceType);
    configAttribs.push_back(EGL_RENDERABLE_TYPE);
    configAttribs.push_back(clientAPI);
  }

  configAttribs.push_back(EGL_NONE);

  EGLint numConfigs = 0;
  EGLConfig config;

  if (eglChooseConfig(this->Display, configAttribs.data(), &config, 1, &numConfigs) == EGL_FALSE ||
    numConfigs < 1)
  {
    vtkLog(ERROR, "eglChooseConfig failed or no matching EGL configurations found!");
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
    this->DestroyOffscreenFramebuffer();
    eglDestroySurface(this->Display, this->Surface);
    this->Surface = EGL_NO_SURFACE;
  }

  this->Config->CreateWindowSurface(
    this->Surface, this->Display, config, this->Width, this->Height);
  if (this->Surface == EGL_NO_SURFACE)
  {
    vtkLog(WARNING, "Failed to create EGL window surface.");
    return;
  }

  if (eglMakeCurrent(this->Display, this->Surface, this->Surface, this->Context) == EGL_FALSE)
  {
    vtkLog(ERROR, "eglMakeCurrent failed!");
    return;
  }

  if (this->IsMesaSoftwareRenderer())
  {
#if !defined(__ANDROID__) && !defined(ANDROID)
    if (!gladLoaderLoadGL())
    {
      vtkLog(ERROR, "Failed to load GL");
      return;
    }
#endif

    this->InitializeOffscreenFramebuffer();
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::DestroyOffscreenFramebuffer()
{
  if (this->SurfacelessFBO == 0 && this->ColorTexture == 0 && this->DepthBuffer == 0)
  {
    return;
  }

  if (this->Display == EGL_NO_DISPLAY || this->Context == EGL_NO_CONTEXT)
  {
    vtkLog(WARNING, "Cannot destroy offscreen framebuffer resources without a valid EGL context.");
    return;
  }

  if (this->Surface != EGL_NO_SURFACE)
  {
    if (eglMakeCurrent(this->Display, this->Surface, this->Surface, this->Context) == EGL_FALSE)
    {
      vtkLog(WARNING, "Failed to make EGL context current while destroying offscreen framebuffer.");
      return;
    }
  }

  if (this->SurfacelessFBO != 0)
  {
    glDeleteFramebuffers(1, &this->SurfacelessFBO);
    this->SurfacelessFBO = 0;
  }
  if (this->ColorTexture != 0)
  {
    glDeleteTextures(1, &this->ColorTexture);
    this->ColorTexture = 0;
  }
  if (this->DepthBuffer != 0)
  {
    glDeleteRenderbuffers(1, &this->DepthBuffer);
    this->DepthBuffer = 0;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::InitializeOffscreenFramebuffer()
{
  this->DestroyOffscreenFramebuffer();
  // Create a framebuffer object for offscreen rendering
  glGenFramebuffers(1, &this->SurfacelessFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, this->SurfacelessFBO);

  glGenTextures(1, &this->ColorTexture);
  glBindTexture(GL_TEXTURE_2D, this->ColorTexture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA8, this->Width, this->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->ColorTexture, 0);

  const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, drawBuffers);

  glGenRenderbuffers(1, &this->DepthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, this->DepthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->Width, this->Height);
  glFramebufferRenderbuffer(
    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->DepthBuffer);
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindowInternals::IsMesaSoftwareRenderer() const
{
#ifdef VTK_USE_MESA_SOFTWARE_RENDERING
  return true;
#else
  return false;
#endif
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindowInternals::GetSizeFromSurface(int* size)
{
  if (this->Display != EGL_NO_DISPLAY && this->Surface != EGL_NO_SURFACE)
  {
    EGLint w, h;
    eglQuerySurface(this->Display, this->Surface, EGL_WIDTH, &w);
    eglQuerySurface(this->Display, this->Surface, EGL_HEIGHT, &h);
    size[0] = w;
    size[1] = h;
  }
  else
  {
    size[0] = this->Width;
    size[1] = this->Height;
  }
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
  if (this->Display == EGL_NO_DISPLAY || this->Context == EGL_NO_CONTEXT)
  {
    return false;
  }
  return eglMakeCurrent(this->Display, this->Surface, this->Surface, this->Context) == EGL_TRUE;
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
