// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEGLRenderWindow.h"

#include "Private/vtkEGLRenderWindowInternals.h"

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkStringScanner.h"
#include "vtkType.h"

#include "vtksys/SystemTools.hxx"

#include "vtk_glad.h"
#include "vtkglad/include/glad/egl.h"

#include <atomic>
#include <sstream>

#if defined(__ANDROID__) || defined(ANDROID)
#include <vtkAndroidRenderWindowInteractor.h>
#endif

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkEGLRenderWindow);

//------------------------------------------------------------------------------
vtkEGLRenderWindow::vtkEGLRenderWindow()
  : Internals(new vtkEGLRenderWindowInternals())
{
  this->OwnWindow = true;
  this->ScreenSize[0] = 1920;
  this->ScreenSize[1] = 1080;

  // this is initialized in vtkRenderWindow
  // so we don't need to initialize on else
  this->DeviceIndex = -1;
#if !defined(__ANDROID__) && !defined(ANDROID)
  this->ShowWindow = false;
#endif

  // If the device index is explicitly set then we need to use it.
  char* EGLDeviceIndexEnv = std::getenv("VTK_EGL_DEVICE_INDEX");
  if (EGLDeviceIndexEnv)
  {
    int index = 0;
    VTK_FROM_CHARS_IF_ERROR_BREAK(EGLDeviceIndexEnv, index);
    if (index >= 0)
    {
      this->DeviceIndex = index;
      this->Internals->SetDeviceIndex(index);
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

//------------------------------------------------------------------------------
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
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::Frame()
{
  this->MakeCurrent();
  this->Superclass::Frame();
  if (this->OwnWindow)
  {
    if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers &&
      this->Internals->GetDisplay() != EGL_NO_DISPLAY)
    {
      eglSwapBuffers(this->Internals->GetDisplay(), this->Internals->GetSurface());
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

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetStereoCapableWindow(vtkTypeBool capable)
{
  if (this->Internals->GetDisplay() == EGL_NO_DISPLAY)
  {
    vtkOpenGLRenderWindow::SetStereoCapableWindow(capable);
  }
  else
  {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetSize(int width, int height)
{
  this->Superclass::SetSize(width, height);

  if (this->OwnWindow && this->Internals->GetDisplay() != EGL_NO_DISPLAY &&
    this->Internals->GetSurface() != EGL_NO_SURFACE)
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkEGLRenderWindow::GetNumberOfDevices()
{
  return this->Internals->GetNumberOfDevices();
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindow::SetDeviceAsDisplay(int deviceIndex)
{

  return this->Internals->SetDeviceAsDisplay(deviceIndex);
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetShowWindow(bool val)
{
  if (val)
  {
#if !defined(USE_WAYLAND) && !defined(__ANDROID__) && !defined(ANDROID)
    vtkWarningMacro(<< "vtkEGLRenderWindow supports onscreen rendering only on Android or with "
                       "Wayland, fallback to offscreen rendering.");
    val = false;
#endif
  }
  else
  {
#if defined(__ANDROID__) || defined(ANDROID)
    vtkWarningMacro(<< "vtkEGLRenderWindow offscreen rendering on Android is not supported, "
                       "fallback to onscreen rendering.");
    val = true;
#endif
  }

  this->Internals->SetUseOnscreenRendering(val);
  this->Superclass::SetShowWindow(val);
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::ResizeWindow(int width, int height)
{
  this->Internals->ConfigureWindow(width, height);

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

  this->Internals->GetSizeFromSurface(this->Size);
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::DestroyWindow()
{
  this->ReleaseGraphicsResources(this);
  if (this->OwnWindow)
  {
    // make sure all other code knows we're not mapped anymore
    this->Mapped = 0;
    this->Internals->DestroyWindow();
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::WindowInitialize()
{
  if (this->OwnWindow)
  {
    this->CreateAWindow();
  }
  else if (this->Internals->GetContext() == EGL_NO_CONTEXT)
  {
    this->Internals->SetContext(eglGetCurrentContext());
  }

  // Initialize OpenGL state
  this->OpenGLInit();

#if !defined(__ANDROID__) && !defined(ANDROID) && defined(GL_POINT_SPRITE)
  if (this->Initialized && !this->ShowWindow)
  {
    glEnable(GL_POINT_SPRITE);
  }
#endif

  // Notify renderers
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
  {
    ren->SetRenderWindow(this);
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::Initialize()
{
  if (this->Internals->GetContext() == EGL_NO_CONTEXT)
  {
    this->WindowInitialize();
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::Finalize()
{
  // clean and destroy window
  this->DestroyWindow();
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetFullScreen(vtkTypeBool vtkNotUsed(arg))
{
  // window is always full screen
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::PrefFullScreen()
{
  // don't show borders
  this->Borders = 0;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set everything up again
  this->Initialize();
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::GetEGLSurfaceSize(int* width, int* height)
{
  if (this->Internals->GetDisplay() != EGL_NO_DISPLAY &&
    this->Internals->GetSurface() != EGL_NO_SURFACE)
  {
    EGLint w, h;
    eglQuerySurface(this->Internals->GetDisplay(), this->Internals->GetSurface(), EGL_WIDTH, &w);
    eglQuerySurface(this->Internals->GetDisplay(), this->Internals->GetSurface(), EGL_HEIGHT, &h);
    *width = w;
    *height = h;
  }
  else
  {
    *width = 0;
    *height = 0;
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Context: " << this->Internals->GetContext() << "\n";
  os << indent << "Display: " << this->Internals->GetDisplay() << "\n";
  os << indent << "Surface: " << this->Internals->GetSurface() << "\n";
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::MakeCurrent()
{
  if (this->Internals->GetDisplay() != EGL_NO_DISPLAY &&
    this->Internals->GetContext() != EGL_NO_CONTEXT &&
    this->Internals->GetSurface() != EGL_NO_SURFACE)
  {
    if (this->Internals->MakeCurrent())
    {
      vtkWarningMacro("Unable to eglMakeCurrent: " << eglGetError());
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::ReleaseCurrent()
{
  this->Internals->ReleaseCurrent();
}

//------------------------------------------------------------------------------
bool vtkEGLRenderWindow::IsCurrent()
{
  return true;
}

//------------------------------------------------------------------------------
int* vtkEGLRenderWindow::GetScreenSize()
{
  // TODO: actually determine screensize.

  return this->ScreenSize;
}

//------------------------------------------------------------------------------
int* vtkEGLRenderWindow::GetPosition()
{
  return this->Position;
}

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
  }
  this->Position[0] = x;
  this->Position[1] = y;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkEGLRenderWindow::SetWindowName(const char* name)
{
  vtkOpenGLRenderWindow::SetWindowName(name);
}

//------------------------------------------------------------------------------
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
  return this->Internals->GetDisplay();
}

//------------------------------------------------------------------------------
void* vtkEGLRenderWindow::GetGenericContext()
{
  return this->Internals->GetContext();
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
  this->Internals->SetWindow(reinterpret_cast<EGLNativeWindowType>(window));
}

//------------------------------------------------------------------------------
const char* vtkEGLRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  if (!this->Internals->GetDisplay())
  {
    return "Display ID not set";
  }
  const char* eglVersion = eglQueryString(this->Internals->GetDisplay(), EGL_VERSION);
  const char* eglVendor = eglQueryString(this->Internals->GetDisplay(), EGL_VENDOR);
  const char* eglClientAPIs = eglQueryString(this->Internals->GetDisplay(), EGL_CLIENT_APIS);
  const char* eglExtensions = eglQueryString(this->Internals->GetDisplay(), EGL_EXTENSIONS);
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
