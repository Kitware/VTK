// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDynamicLoader.h"
#include "vtk_glad.h"

#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif

#include "vtkOSOpenGLRenderWindow.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"

#include "vtksys/SystemTools.hxx"
#include <sstream>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkOSOpenGLRenderWindow;
class vtkRenderWindow;

#define OSMESA_MAJOR_VERSION 11
#define OSMESA_MINOR_VERSION 2
#define OSMESA_PATCH_VERSION 0

/*
 * Values for the format parameter of OSMesaCreateContext()
 * New in version 2.0.
 */
#define OSMESA_COLOR_INDEX GL_COLOR_INDEX
#define OSMESA_RGBA GL_RGBA
#define OSMESA_BGRA 0x1
#define OSMESA_ARGB 0x2
#define OSMESA_RGB GL_RGB
#define OSMESA_BGR 0x4
#define OSMESA_RGB_565 0x5

/*
 * OSMesaPixelStore() parameters:
 * New in version 2.0.
 */
#define OSMESA_ROW_LENGTH 0x10
#define OSMESA_Y_UP 0x11

/*
 * Accepted by OSMesaGetIntegerv:
 */
#define OSMESA_WIDTH 0x20
#define OSMESA_HEIGHT 0x21
#define OSMESA_FORMAT 0x22
#define OSMESA_TYPE 0x23
#define OSMESA_MAX_WIDTH 0x24  /* new in 4.0 */
#define OSMESA_MAX_HEIGHT 0x25 /* new in 4.0 */

/*
 * Accepted in OSMesaCreateContextAttrib's attribute list.
 */
#define OSMESA_DEPTH_BITS 0x30
#define OSMESA_STENCIL_BITS 0x31
#define OSMESA_ACCUM_BITS 0x32
#define OSMESA_PROFILE 0x33
#define OSMESA_CORE_PROFILE 0x34
#define OSMESA_COMPAT_PROFILE 0x35
#define OSMESA_CONTEXT_MAJOR_VERSION 0x36
#define OSMESA_CONTEXT_MINOR_VERSION 0x37

typedef struct osmesa_context* OSMesaContext;

class vtkOSOpenGLRenderWindowInternal
{
  friend class vtkOSOpenGLRenderWindow;

  typedef OSMesaContext(GLAPIENTRY* PFNOSMesaCreateContext)(GLenum format, OSMesaContext sharelist);
  typedef OSMesaContext(GLAPIENTRY* PFNOSMesaCreateContextAttribs)(
    const int* attribList, OSMesaContext sharelist);
  typedef void(GLAPIENTRY* PFNOSMesaDestroyContext)(OSMesaContext ctx);
  typedef GLboolean(GLAPIENTRY* PFNOSMesaMakeCurrent)(
    OSMesaContext ctx, void* buffer, GLenum type, GLsizei width, GLsizei height);
  typedef OSMesaContext(GLAPIENTRY* PFNOSMesaGetCurrentContext)();

  typedef void (*OSMESAproc)();
  typedef OSMESAproc(GLAPIENTRY* PFNOSMesaGetProcAddress)(const char* funcName);

  PFNOSMesaCreateContext OSMesaCreateContext;
  PFNOSMesaCreateContextAttribs OSMesaCreateContextAttribs;
  PFNOSMesaDestroyContext OSMesaDestroyContext;
  PFNOSMesaMakeCurrent OSMesaMakeCurrent;
  PFNOSMesaGetCurrentContext OSMesaGetCurrentContext;
  PFNOSMesaGetProcAddress OSMesaGetProcAddress;

  static vtkLibHandle OSMesaLibraryHandle;

private:
  vtkOSOpenGLRenderWindowInternal();

  // OffScreen stuff
  OSMesaContext OffScreenContextId;
  void* OffScreenWindow;
};

vtkLibHandle vtkOSOpenGLRenderWindowInternal::OSMesaLibraryHandle = nullptr;

vtkOSOpenGLRenderWindowInternal::vtkOSOpenGLRenderWindowInternal()
{
  // OpenGL specific
  this->OffScreenContextId = nullptr;
  this->OffScreenWindow = nullptr;

#if defined(_WIN32)
  OSMesaLibraryHandle = LoadLibraryA("osmesa.dll");
  if (OSMesaLibraryHandle == nullptr)
  {
    vtkGenericWarningMacro(
      << "osmesa.dll not found. It appears that OSMesa is not installed in "
         "your system. Please install the OSMesa library. You can obtain pre-built binaries for "
         "Windows from https://github.com/pal1000/mesa-dist-win. Ensure that osmesa.dll is "
         "available in PATH.");
  }
#elif defined(__linux__)
  const std::vector<std::string> libNamesToTry = { "libOSMesa.so.8", "libOSMesa.so.6",
    "libOSMesa.so" };
  for (const auto& libName : libNamesToTry)
  {
    OSMesaLibraryHandle = dlopen(libName.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (OSMesaLibraryHandle)
    {
      break;
    }
  }
  if (OSMesaLibraryHandle == nullptr)
  {
    vtkGenericWarningMacro(<< "libOSMesa not found. It appears that OSMesa is not installed in "
                              "your system. Please install the OSMesa library from your "
                              "distribution's package manager.");
  }
#else
  vtkGenericWarningMacro(<< "VTK does not support OSMesa for your operating system."
                            "Please create an issue requesting osmesa support - "
                            "https://gitlab.kitware.com/vtk/vtk/-/issues/new");
#endif

  this->OSMesaCreateContext = (PFNOSMesaCreateContext)vtkDynamicLoader::GetSymbolAddress(
    OSMesaLibraryHandle, "OSMesaCreateContext");
  this->OSMesaCreateContextAttribs =
    (PFNOSMesaCreateContextAttribs)vtkDynamicLoader::GetSymbolAddress(
      OSMesaLibraryHandle, "OSMesaCreateContextAttribs");
  this->OSMesaDestroyContext = (PFNOSMesaDestroyContext)vtkDynamicLoader::GetSymbolAddress(
    OSMesaLibraryHandle, "OSMesaDestroyContext");
  this->OSMesaMakeCurrent = (PFNOSMesaMakeCurrent)vtkDynamicLoader::GetSymbolAddress(
    OSMesaLibraryHandle, "OSMesaMakeCurrent");
  this->OSMesaGetCurrentContext = (PFNOSMesaGetCurrentContext)vtkDynamicLoader::GetSymbolAddress(
    OSMesaLibraryHandle, "OSMesaGetCurrentContext");
  this->OSMesaGetProcAddress = (PFNOSMesaGetProcAddress)vtkDynamicLoader::GetSymbolAddress(
    OSMesaLibraryHandle, "OSMesaGetProcAddress");
}

vtkStandardNewMacro(vtkOSOpenGLRenderWindow);

// a couple of routines for offscreen rendering
void vtkOSMesaDestroyWindow(void* Window)
{
  free(Window);
}

void* vtkOSMesaCreateWindow(int width, int height)
{
  return malloc(width * height * 4);
}

vtkOSOpenGLRenderWindow::vtkOSOpenGLRenderWindow()
{
  //   this->ParentId = nullptr;
  this->ScreenSize[0] = 1280;
  this->ScreenSize[1] = 1024;
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->OwnWindow = 0;
  this->ShowWindow = false;
  this->UseOffScreenBuffers = true;

  this->Internal = new vtkOSOpenGLRenderWindowInternal();
  this->SetOpenGLSymbolLoader(
    [](void* userData, const char* name) -> VTKOpenGLAPIProc
    {
      if (auto* internal = reinterpret_cast<vtkOSOpenGLRenderWindowInternal*>(userData))
      {
        return internal->OSMesaGetProcAddress(name);
      }
      return nullptr;
    },
    this->Internal);
}

// free up memory & close the window
vtkOSOpenGLRenderWindow::~vtkOSOpenGLRenderWindow()
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

  delete this->Internal;
}

// End the rendering process and display the image.
void vtkOSOpenGLRenderWindow::Frame()
{
  this->MakeCurrent();
  this->Superclass::Frame();
}

//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkOSOpenGLRenderWindow::SetStereoCapableWindow(vtkTypeBool capable)
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
  this->CreateOffScreenWindow(this->Size[0], this->Size[1]);
}

void vtkOSOpenGLRenderWindow::DestroyWindow()
{
  this->MakeCurrent();
  this->ReleaseGraphicsResources(this);

  delete[] this->Capabilities;
  this->Capabilities = nullptr;

  this->DestroyOffScreenWindow();

  // make sure all other code knows we're not mapped anymore
  this->Mapped = 0;
}

void vtkOSOpenGLRenderWindow::CreateOffScreenWindow(int width, int height)
{
  if (!this->Internal->OSMesaCreateContext || !this->Internal->OSMesaCreateContextAttribs)
  {
    return;
  }
  this->DoubleBuffer = 0;

  if (!this->Internal->OffScreenWindow)
  {
    this->Internal->OffScreenWindow = vtkOSMesaCreateWindow(width, height);
    this->OwnWindow = 1;
  }
#if (OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 1102) &&                                 \
  defined(OSMESA_CONTEXT_MAJOR_VERSION)
  if (!this->Internal->OffScreenContextId)
  {
    static const int attribs[] = { OSMESA_FORMAT, OSMESA_RGBA, OSMESA_DEPTH_BITS, 32,
      OSMESA_STENCIL_BITS, 0, OSMESA_ACCUM_BITS, 0, OSMESA_PROFILE, OSMESA_CORE_PROFILE,
      OSMESA_CONTEXT_MAJOR_VERSION, 3, OSMESA_CONTEXT_MINOR_VERSION, 2, 0 };

    if (this->Internal->OSMesaCreateContextAttribs != nullptr)
    {
      this->Internal->OffScreenContextId =
        this->Internal->OSMesaCreateContextAttribs(attribs, nullptr);
    }
  }
#endif
  // if we still have no context fall back to the generic signature
  if (!this->Internal->OffScreenContextId)
  {
    this->Internal->OffScreenContextId = this->Internal->OSMesaCreateContext(GL_RGBA, nullptr);
  }

  this->Mapped = 0;
  this->Size[0] = width;
  this->Size[1] = height;

  this->MakeCurrent();

  // tell our renderers about us
  vtkRenderer* ren;
  for (this->Renderers->InitTraversal(); (ren = this->Renderers->GetNextItem());)
  {
    ren->SetRenderWindow(nullptr);
    ren->SetRenderWindow(this);
  }

  this->OpenGLInit();
}

void vtkOSOpenGLRenderWindow::DestroyOffScreenWindow()
{
  // Release graphic resources.

  // First release graphics resources on the window itself
  // since call to Renderer's SetRenderWindow(nullptr), just
  // calls ReleaseGraphicsResources on vtkProps. And also
  // this call invokes Renderer's ReleaseGraphicsResources
  // method which only invokes ReleaseGraphicsResources on
  // rendering passes.
  this->ReleaseGraphicsResources(this);

  if (this->Internal->OffScreenContextId)
  {
    this->Internal->OSMesaDestroyContext(this->Internal->OffScreenContextId);
    this->Internal->OffScreenContextId = nullptr;
    vtkOSMesaDestroyWindow(this->Internal->OffScreenWindow);
    this->Internal->OffScreenWindow = nullptr;
  }
}

void vtkOSOpenGLRenderWindow::ResizeOffScreenWindow(int width, int height)
{
  auto& internal = (*this->Internal);
  if (internal.OffScreenContextId)
  {
    // in past, we used to destroy the context and recreate one on resize. this
    // is totally unnecessary; we just recreate the buffer and make it current.
    vtkOSMesaDestroyWindow(internal.OffScreenWindow);
    internal.OffScreenWindow = vtkOSMesaCreateWindow(width, height);

    // Call MakeCurrent to ensure that we're no longer using the old memory
    // buffer.
    this->MakeCurrent();
  }
}

// Initialize the window for rendering.
void vtkOSOpenGLRenderWindow::WindowInitialize()
{
  this->CreateAWindow();

  if (!this->Internal->OffScreenContextId)
  {
    return;
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
}

// Initialize the rendering window.
void vtkOSOpenGLRenderWindow::Initialize()
{
  if (!(this->Internal->OffScreenContextId))
  {
    // initialize offscreen window
    int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
    int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
    this->CreateOffScreenWindow(width, height);
  }
}

void vtkOSOpenGLRenderWindow::Finalize()
{
  // clean and destroy window
  this->DestroyWindow();
}

// Change the window to fill the entire screen.
void vtkOSOpenGLRenderWindow::SetFullScreen(vtkTypeBool arg)
{
  (void)arg;
  this->Modified();
}

// Resize the window.
void vtkOSOpenGLRenderWindow::WindowRemap()
{
  // shut everything down
  this->Finalize();

  // set everything up again
  this->Initialize();
}

// Specify the size of the rendering window.
void vtkOSOpenGLRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Superclass::SetSize(width, height);
    this->ResizeOffScreenWindow(width, height);
    this->Modified();
  }
}

void vtkOSOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OffScreenContextId: " << this->Internal->OffScreenContextId << "\n";
}

void vtkOSOpenGLRenderWindow::MakeCurrent()
{
  // set the current window
  if (this->Internal->OffScreenContextId)
  {
    if (this->Internal->OSMesaMakeCurrent(this->Internal->OffScreenContextId,
          this->Internal->OffScreenWindow, GL_UNSIGNED_BYTE, this->Size[0],
          this->Size[1]) != GL_TRUE)
    {
      vtkWarningMacro("failed call to OSMesaMakeCurrent");
    }
  }
}

//------------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOSOpenGLRenderWindow::IsCurrent()
{
  bool result = false;
  if (this->Internal->OffScreenContextId)
  {
    result = this->Internal->OffScreenContextId == this->Internal->OSMesaGetCurrentContext();
  }
  return result;
}

void vtkOSOpenGLRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

void* vtkOSOpenGLRenderWindow::GetGenericContext()
{
  return (void*)this->Internal->OffScreenContextId;
}

vtkTypeBool vtkOSOpenGLRenderWindow::GetEventPending()
{
  return 0;
}

// Get the size of the screen in pixels
int* vtkOSOpenGLRenderWindow::GetScreenSize()
{
  this->ScreenSize[0] = 1280;
  this->ScreenSize[1] = 1024;
  return this->ScreenSize;
}

// Get the position in screen coordinates (pixels) of the window.
int* vtkOSOpenGLRenderWindow::GetPosition()
{
  return this->Position;
}

// Move the window to a new position on the display.
void vtkOSOpenGLRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
  }
  this->Position[0] = x;
  this->Position[1] = y;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkOSOpenGLRenderWindow::SetWindowInfo(const char* info)
{
  int tmp;

  this->OwnDisplay = 1;

  sscanf(info, "%i", &tmp);
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkOSOpenGLRenderWindow::SetNextWindowInfo(const char* info)
{
  int tmp;
  sscanf(info, "%i", &tmp);

  //   this->SetNextWindowId((Window)tmp);
}

// Sets the X window id of the window that WILL BE created.
void vtkOSOpenGLRenderWindow::SetParentInfo(const char* info)
{
  int tmp;

  // get the default display connection
  this->OwnDisplay = 1;

  sscanf(info, "%i", &tmp);

  //   this->SetParentId(tmp);
}

void vtkOSOpenGLRenderWindow::SetWindowId(void* arg)
{
  (void)arg;
  //   this->SetWindowId((Window)arg);
}
void vtkOSOpenGLRenderWindow::SetParentId(void* arg)
{
  (void)arg;
  //   this->SetParentId((Window)arg);
}

const char* vtkOSOpenGLRenderWindow::ReportCapabilities()
{
  MakeCurrent();

  //   int scrnum = DefaultScreen(this->DisplayId);

  const char* glVendor = (const char*)glGetString(GL_VENDOR);
  const char* glRenderer = (const char*)glGetString(GL_RENDERER);
  const char* glVersion = (const char*)glGetString(GL_VERSION);

  std::ostringstream strm;
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

vtkTypeBool vtkOSOpenGLRenderWindow::IsDirect()
{
  MakeCurrent();
  return 0;
}

void vtkOSOpenGLRenderWindow::SetWindowName(const char* cname)
{
  char* name = new char[strlen(cname) + 1];
  strcpy(name, cname);
  vtkOpenGLRenderWindow::SetWindowName(name);
  delete[] name;
}

void vtkOSOpenGLRenderWindow::SetNextWindowId(void* arg)
{
  (void)arg;
}

// This probably has been moved to superclass.
void* vtkOSOpenGLRenderWindow::GetGenericWindowId()
{
  return this->Internal->OffScreenWindow;
}
VTK_ABI_NAMESPACE_END
