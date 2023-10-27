// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSDL2OpenGLRenderWindow.h"

#include "vtkCommand.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"

#include <cmath>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSDL2OpenGLRenderWindow);

const std::string vtkSDL2OpenGLRenderWindow::DEFAULT_BASE_WINDOW_NAME =
  "Visualization Toolkit - SDL2OpenGL #";

vtkSDL2OpenGLRenderWindow::vtkSDL2OpenGLRenderWindow()
  : WindowId(nullptr)
  , ContextId(nullptr)
{
  this->SetWindowName(DEFAULT_BASE_WINDOW_NAME.c_str());
  this->SetStencilCapable(1);

  // set position to -1 to let SDL place the window
  // SetPosition will still work. Defaults of 0,0 result
  // in the window title bar being off screen.
  this->Position[0] = -1;
  this->Position[1] = -1;

  this->FrameBlitMode = BlitToCurrent;
}

vtkSDL2OpenGLRenderWindow::~vtkSDL2OpenGLRenderWindow()
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

void vtkSDL2OpenGLRenderWindow::Clean()
{
  /* finish OpenGL rendering */
  if (this->OwnContext && this->ContextId)
  {
    this->MakeCurrent();
    this->CleanUpRenderers();
    SDL_GL_DeleteContext(this->ContextId);
  }
  this->ContextId = nullptr;
}

void vtkSDL2OpenGLRenderWindow::CleanUpRenderers()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  this->ReleaseGraphicsResources(this);
}

void vtkSDL2OpenGLRenderWindow::SetWindowName(const char* title)
{
  this->Superclass::SetWindowName(title);
  if (this->WindowId)
  {
    SDL_SetWindowTitle(this->WindowId, title);
  }
}

//------------------------------------------------------------------------------
void vtkSDL2OpenGLRenderWindow::MakeCurrent()
{
  if (this->ContextId)
  {
    SDL_GL_MakeCurrent(this->WindowId, this->ContextId);
  }
}

void vtkSDL2OpenGLRenderWindow::ReleaseCurrent()
{
  if (this->ContextId)
  {
    SDL_GL_MakeCurrent(this->WindowId, nullptr);
  }
}

void vtkSDL2OpenGLRenderWindow::PushContext()
{
  SDL_GLContext current = SDL_GL_GetCurrentContext();
  this->ContextStack.push(current);
  this->WindowStack.push(SDL_GL_GetCurrentWindow());
  if (current != this->ContextId)
  {
    this->MakeCurrent();
  }
}

void vtkSDL2OpenGLRenderWindow::PopContext()
{
  SDL_GLContext current = SDL_GL_GetCurrentContext();
  SDL_GLContext target = this->ContextStack.top();
  SDL_Window* win = this->WindowStack.top();
  this->ContextStack.pop();
  this->WindowStack.pop();
  if (target != current)
  {
    SDL_GL_MakeCurrent(win, target);
  }
}

//------------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkSDL2OpenGLRenderWindow::IsCurrent()
{
  return this->ContextId != 0 && this->ContextId == SDL_GL_GetCurrentContext();
}

bool vtkSDL2OpenGLRenderWindow::SetSwapControl(int i)
{
  SDL_GL_SetSwapInterval(i);
  return true;
}

//------------------------------------------------------------------------------
void vtkSDL2OpenGLRenderWindow::SetSize(int x, int y)
{
  if ((this->Size[0] != x) || (this->Size[1] != y))
  {
    if (this->WindowId)
    {
      SDL_SetWindowSize(this->WindowId, x, y);
    }
    // For high dpi screens, SDL2 recommends querying the GL drawable size.
    // This is needed for the glViewport call in vtkOpenGLCamera::Render to
    // work correctly.
    // See https://wiki.libsdl.org/SDL2/SDL_GL_GetDrawableSize
    if (this->GetDPI() > 72)
    {
      SDL_GL_GetDrawableSize(this->WindowId, &this->Size[0], &this->Size[1]);
    }
    else
    {
      SDL_GetWindowSize(this->WindowId, &this->Size[0], &this->Size[1]);
    }
    if (this->Interactor)
    {
      this->Interactor->SetSize(this->Size[0], this->Size[1]);
    }
    this->Modified();
    this->InvokeEvent(vtkCommand::WindowResizeEvent, nullptr);
  }
}

void vtkSDL2OpenGLRenderWindow::SetPosition(int x, int y)
{
  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->Mapped)
    {
      SDL_SetWindowPosition(this->WindowId, x, y);
    }
  }
}

void vtkSDL2OpenGLRenderWindow::Frame()
{
  this->Superclass::Frame();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    SDL_GL_SwapWindow(this->WindowId);
  }
}

int vtkSDL2OpenGLRenderWindow::GetColorBufferSizes(int* rgba)
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

void vtkSDL2OpenGLRenderWindow::SetShowWindow(bool val)
{
  if (val == this->ShowWindow)
  {
    return;
  }

  if (this->WindowId)
  {
    if (val)
    {
      SDL_ShowWindow(this->WindowId);
    }
    else
    {
      SDL_HideWindow(this->WindowId);
    }
    this->Mapped = val;
  }
  this->Superclass::SetShowWindow(val);
}

void vtkSDL2OpenGLRenderWindow::CreateAWindow()
{
  int x = ((this->Position[0] >= 0) ? this->Position[0] : SDL_WINDOWPOS_UNDEFINED);
  int y = ((this->Position[1] >= 0) ? this->Position[1] : SDL_WINDOWPOS_UNDEFINED);
  int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  this->SetSize(width, height);

#ifdef __EMSCRIPTEN__
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif

  this->WindowId = SDL_CreateWindow(this->WindowName, x, y, width, height,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_SetWindowResizable(this->WindowId, SDL_TRUE);
  if (this->WindowId)
  {
    int idx = SDL_GetWindowDisplayIndex(this->WindowId);
    float hdpi = 72.0;
    SDL_GetDisplayDPI(idx, nullptr, &hdpi, nullptr);
    this->SetDPI(hdpi);
  }
}

// Initialize the rendering window.
void vtkSDL2OpenGLRenderWindow::Initialize()
{
  int res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

  if (res)
  {
    vtkErrorMacro("Error initializing SDL " << SDL_GetError());
  }

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);

#ifdef GL_ES_VERSION_3_0
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

  if (!this->WindowId)
  {
    this->CreateAWindow();
  }

  if (!this->ContextId)
  {
    this->ContextId = SDL_GL_CreateContext(this->WindowId);
  }
  if (!this->ContextId)
  {
    vtkErrorMacro("Unable to create SDL2 opengl context");
  }
  this->OpenGLInit();
}

void vtkSDL2OpenGLRenderWindow::Finalize()
{
  this->DestroyWindow();
}

void vtkSDL2OpenGLRenderWindow::DestroyWindow()
{
  this->Clean();
  if (this->WindowId)
  {
    SDL_DestroyWindow(this->WindowId);
    this->WindowId = nullptr;
  }
}

// Get the current size of the window.
int* vtkSDL2OpenGLRenderWindow::GetSize(void)
{
  if (this->WindowId)
  {
    int w = 0;
    int h = 0;
    if (this->GetDPI() > 72)
    {
      SDL_GL_GetDrawableSize(this->WindowId, &w, &h);
      this->Size[0] = w;
      this->Size[1] = h;
    }
    else
    {
      SDL_GetWindowSize(this->WindowId, &w, &h);
      this->Size[0] = w;
      this->Size[1] = h;
    }
  }

  return this->vtkOpenGLRenderWindow::GetSize();
}

// Get the size of the whole screen.
int* vtkSDL2OpenGLRenderWindow::GetScreenSize(void)
{
  SDL_Rect rect;
  SDL_GetDisplayBounds(0, &rect);
  this->Size[0] = rect.w;
  this->Size[1] = rect.h;

  return this->Size;
}

// Get the position in screen coordinates of the window.
int* vtkSDL2OpenGLRenderWindow::GetPosition(void)
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
  {
    return this->Position;
  }

  //  Find the current window position
  SDL_GetWindowPosition(this->WindowId, &this->Position[0], &this->Position[1]);

  return this->Position;
}

// Change the window to fill the entire screen.
void vtkSDL2OpenGLRenderWindow::SetFullScreen(vtkTypeBool arg)
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
  SDL_SetWindowFullscreen(this->WindowId, arg ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  this->Modified();
}

void vtkSDL2OpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}

//------------------------------------------------------------------------------
void vtkSDL2OpenGLRenderWindow::HideCursor()
{
  SDL_ShowCursor(SDL_DISABLE);
}

//------------------------------------------------------------------------------
void vtkSDL2OpenGLRenderWindow::ShowCursor()
{
  SDL_ShowCursor(SDL_ENABLE);
}
VTK_ABI_NAMESPACE_END
