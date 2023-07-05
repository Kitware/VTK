// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSDL2WebGPURenderWindow.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkWGPUContext.h"
// Ignore reserved-identifier warnings from
// 1. SDL2/SDL_stdinc.h: warning: identifier '_SDL_size_mul_overflow_builtin'
// 2. SDL2/SDL_stdinc.h: warning: identifier '_SDL_size_add_overflow_builtin'
// 3. SDL2/SDL_audio.h: warning: identifier '_SDL_AudioStream'
// 4. SDL2/SDL_joystick.h: warning: identifier '_SDL_Joystick'
// 5. SDL2/SDL_sensor.h: warning: identifier '_SDL_Sensor'
// 6. SDL2/SDL_gamecontroller.h: warning: identifier '_SDL_GameController'
// 7. SDL2/SDL_haptic.h: warning: identifier '_SDL_Haptic'
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "SDL.h"
#include "SDL_syswm.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef VTK_DAWN_ENABLE_BACKEND_METAL
#include "wgpu_utils_metal.h"
#endif

VTK_ABI_NAMESPACE_BEGIN
const std::string vtkSDL2WebGPURenderWindow::DEFAULT_BASE_WINDOW_NAME =
  "Visualization Toolkit - SDL2 WebGPU #";

namespace
{
SDL_Window* ToSDLWindow(void* window)
{
  return reinterpret_cast<SDL_Window*>(window);
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSDL2WebGPURenderWindow);

//------------------------------------------------------------------------------
vtkSDL2WebGPURenderWindow::vtkSDL2WebGPURenderWindow()
{
  this->SetWindowName(DEFAULT_BASE_WINDOW_NAME.c_str());
  this->SetStencilCapable(1);

  // set position to -1 to let SDL place the window
  // SetPosition will still work. Defaults of 0,0 result
  // in the window title bar being off screen.
  this->Position[0] = -1;
  this->Position[1] = -1;
}

//------------------------------------------------------------------------------
vtkSDL2WebGPURenderWindow::~vtkSDL2WebGPURenderWindow()
{
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
void vtkSDL2WebGPURenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  os << this->WindowId << '\n';
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkSDL2WebGPURenderWindow::Initialize()
{
  int res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  if (res)
  {
    vtkErrorMacro("Error initializing SDL " << SDL_GetError());
  }
  if (!this->WindowId)
  {
    this->CreateAWindow();
  }
  if (this->WGPUInit())
  {
#ifdef __EMSCRIPTEN__
    // render into canvas elememnt
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector htmlSurfDesc;
    htmlSurfDesc.selector = "#canvas";
    this->Surface = vtkWGPUContext::CreateSurface(htmlSurfDesc);
    return this->Surface.Get() != nullptr;
#else
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version)
    if (SDL_GetWindowWMInfo(ToSDLWindow(this->WindowId), &wmInfo))
    {
#ifdef VTK_DAWN_ENABLE_BACKEND_D3D12
      if (wmInfo.subsystem == SDL_SYSWM_WINDOWS)
      {
        wgpu::SurfaceDescriptorFromWindowsHWND winSurfDesc;
        winSurfDesc.hwnd = wmInfo.info.win.window;
        winSurfDesc.hinstance = wmInfo.info.win.hinstance;
        this->Surface = vtkWGPUContext::CreateSurface(winSurfDesc);
        return true;
      }
#elif defined(VTK_DAWN_ENABLE_BACKEND_METAL)
      if (wmInfo.subsystem == SDL_SYSWM_COCOA)
      {
        auto cocoaSurfDesc = SetupWindowAndGetSurfaceDescriptorCocoa(wmInfo.info.cocoa.window);
        this->Surface = vtkWGPUContext::CreateSurface(*cocoaSurfDesc);
        return true;
      }
#elif defined(VTK_DAWN_USE_WAYLAND)
      if (wmInfo.subsystem == SDL_SYSWM_WAYLAND)
      {
        wgpu::SurfaceDescriptorFromWaylandSurface wlSurfDesc;
        wlSurfDesc.display = wmInfo.info.wl.display;
        wlSurfDesc.surface = wmInfo.info.wl.surface;
        this->Surface = vtkWGPUContext::CreateSurface(wlSurfDesc);
        return true;
      }
      else if (wmInfo.subsystem == SDL_SYSWM_X11)
#elif defined(VTK_DAWN_USE_X11)
      {
        wgpu::SurfaceDescriptorFromXlibWindow x11SurfDesc;
        x11SurfDesc.display = wmInfo.info.x11.display;
        x11SurfDesc.window = wmInfo.info.x11.window;
        this->Surface = vtkWGPUContext::CreateSurface(x11SurfDesc);
        return true;
      }
#else
      return false;
#endif
    }
#endif
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::Finalize()
{
  if (this->WGPUInitialized)
  {
    this->WGPUFinalize();
  }
  this->DestroyWindow();
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::SetFullScreen(vtkTypeBool arg)
{
  if (this->FullScreen == arg)
  {
    return;
  }

  if (!this->Mapped)
  {
    return;
  }

  // set the mode
  this->FullScreen = arg;
  SDL_SetWindowFullscreen(ToSDLWindow(this->WindowId), arg ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::SetShowWindow(bool val)
{
  if (val == this->ShowWindow)
  {
    return;
  }

  if (this->WindowId)
  {
    if (val)
    {
      SDL_ShowWindow(ToSDLWindow(this->WindowId));
    }
    else
    {
      SDL_HideWindow(ToSDLWindow(this->WindowId));
    }
    this->Mapped = val;
  }
  this->Superclass::SetShowWindow(val);
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::SetSize(int w, int h)
{
  if ((this->Size[0] != w) || (this->Size[1] != h))
  {
    this->Superclass::SetSize(w, h);

    if (this->Interactor)
    {
      this->Interactor->SetSize(w, h);
    }
    if (this->WindowId)
    {
      int currentW, currentH;
      SDL_GetWindowSize(ToSDLWindow(this->WindowId), &currentW, &currentH);
      // set the size only when window is programatically resized.
      if (currentW != w || currentH != h)
      {
        SDL_SetWindowSize(ToSDLWindow(this->WindowId), w, h);
      }
    }
  }
}

//------------------------------------------------------------------------------
int* vtkSDL2WebGPURenderWindow::GetSize()
{
  // if we aren't mapped then just return the ivar
  if (this->WindowId && this->Mapped)
  {
    int w = 0;
    int h = 0;

    SDL_GetWindowSize(ToSDLWindow(this->WindowId), &w, &h);
    this->Size[0] = w;
    this->Size[1] = h;
  }

  return this->Superclass::GetSize();
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->Mapped)
    {
      SDL_SetWindowPosition(ToSDLWindow(this->WindowId), x, y);
    }
  }
}

//------------------------------------------------------------------------------
int* vtkSDL2WebGPURenderWindow::GetScreenSize()
{
  SDL_Rect rect;
  SDL_GetDisplayBounds(0, &rect);
  this->Size[0] = rect.w;
  this->Size[1] = rect.h;

  return this->Size;
}

//------------------------------------------------------------------------------
int* vtkSDL2WebGPURenderWindow::GetPosition()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
  {
    return this->Position;
  }

  //  Find the current window position
  SDL_GetWindowPosition(ToSDLWindow(this->WindowId), &this->Position[0], &this->Position[1]);

  return this->Position;
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::SetWindowName(const char* title)
{
  this->Superclass::SetWindowName(title);
  if (this->WindowId)
  {
    SDL_SetWindowTitle(ToSDLWindow(this->WindowId), title);
  }
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::Clean()
{
  this->CleanUpRenderers();
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::Frame()
{
  if (!this->AbortRender)
  {
    this->Superclass::Frame();
  }
}

//------------------------------------------------------------------------------
int vtkSDL2WebGPURenderWindow::GetColorBufferSizes(int* rgba)
{
  if (rgba == nullptr)
  {
    return 0;
  }
  rgba[0] = 8;
  rgba[1] = 8;
  rgba[2] = 8;
  rgba[3] = 8;
  return 1;
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::HideCursor()
{
  SDL_ShowCursor(SDL_DISABLE);
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::ShowCursor()
{
  SDL_ShowCursor(SDL_ENABLE);
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::CleanUpRenderers()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  this->ReleaseGraphicsResources(this);
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::CreateAWindow()
{
  int x = ((this->Position[0] >= 0) ? this->Position[0] : SDL_WINDOWPOS_UNDEFINED);
  int y = ((this->Position[1] >= 0) ? this->Position[1] : SDL_WINDOWPOS_UNDEFINED);
  int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  this->SetSize(width, height);

#ifdef __EMSCRIPTEN__
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif

  this->WindowId = SDL_CreateWindow(this->WindowName, x, y, width, height, SDL_WINDOW_RESIZABLE);
  SDL_SetWindowResizable(ToSDLWindow(this->WindowId), SDL_TRUE);
  if (this->WindowId)
  {
    int idx = SDL_GetWindowDisplayIndex(ToSDLWindow(this->WindowId));
    float hdpi = 72.0;
    SDL_GetDisplayDPI(idx, nullptr, &hdpi, nullptr);
    this->SetDPI(hdpi);
  }
}

//------------------------------------------------------------------------------
void vtkSDL2WebGPURenderWindow::DestroyWindow()
{
  this->Clean();
  if (this->WindowId)
  {
    SDL_DestroyWindow(ToSDLWindow(this->WindowId));
    this->WindowId = nullptr;
  }
}

VTK_ABI_NAMESPACE_END
