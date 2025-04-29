// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebAssemblyOpenGLRenderWindow.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <webgl/webgl2.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebAssemblyOpenGLRenderWindow);

//------------------------------------------------------------------------------
vtkWebAssemblyOpenGLRenderWindow::vtkWebAssemblyOpenGLRenderWindow()
  : ContextId(0)
{
  this->SetWindowName("Visualization Toolkit - Emscripten OpenGL #");
  this->SetCanvasSelector("#canvas");
  this->SetStencilCapable(1);

  this->Position[0] = -1;
  this->Position[1] = -1;

  this->FrameBlitMode = BlitToCurrent;
}

//------------------------------------------------------------------------------
vtkWebAssemblyOpenGLRenderWindow::~vtkWebAssemblyOpenGLRenderWindow()
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
void vtkWebAssemblyOpenGLRenderWindow::Clean()
{
  if (this->OwnContext && this->ContextId)
  {
    this->MakeCurrent();
    this->CleanUpRenderers();
    emscripten_webgl_destroy_context(this->ContextId);
  }
  this->ContextId = 0;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::CleanUpRenderers()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  this->ReleaseGraphicsResources(this);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::MakeCurrent()
{
  if (this->ContextId)
  {
    emscripten_webgl_make_context_current(this->ContextId);
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::ReleaseCurrent()
{
  if (this->ContextId)
  {
    emscripten_webgl_make_context_current(0);
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::PushContext()
{
  auto current = emscripten_webgl_get_current_context();
  this->ContextStack.push(current);
  if (current != this->ContextId)
  {
    this->MakeCurrent();
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::PopContext()
{
  auto current = emscripten_webgl_get_current_context();
  auto target = this->ContextStack.top();
  this->ContextStack.pop();
  if (target != current)
  {
    emscripten_webgl_make_context_current(target);
  }
}

//------------------------------------------------------------------------------
bool vtkWebAssemblyOpenGLRenderWindow::IsCurrent()
{
  return this->ContextId != 0 && this->ContextId == emscripten_webgl_get_current_context();
}

//------------------------------------------------------------------------------
bool vtkWebAssemblyOpenGLRenderWindow::SetSwapControl(int interval)
{
  if (interval < 0)
  {
    vtkWarningMacro("Late swap tearing currently unsupported!");
    return false;
  }
  else if (interval == 0)
  {
    emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, 0);
  }
  else
  {
    emscripten_set_main_loop_timing(EM_TIMING_RAF, interval);
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Size[0] = width;
    this->Size[1] = height;
    emscripten_set_canvas_element_size(this->CanvasSelector, this->Size[0], this->Size[1]);
    if (this->Interactor)
    {
      this->Interactor->SetSize(this->Size[0], this->Size[1]);
    }
    this->Modified();
    this->InvokeEvent(vtkCommand::WindowResizeEvent, nullptr);
  }
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::Frame()
{
  this->Superclass::Frame();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    if (emscripten_has_asyncify())
    {
      // give back control to browser for screen refresh */
      emscripten_sleep(0);
    }
  }
}

//------------------------------------------------------------------------------
int vtkWebAssemblyOpenGLRenderWindow::GetColorBufferSizes(int* rgba)
{
  if (rgba == nullptr)
  {
    return 0;
  }
  rgba[0] = emscripten_webgl_get_parameter_d(GL_RED_BITS);
  rgba[1] = emscripten_webgl_get_parameter_d(GL_GREEN_BITS);
  rgba[2] = emscripten_webgl_get_parameter_d(GL_BLUE_BITS);
  rgba[3] = emscripten_webgl_get_parameter_d(GL_ALPHA_BITS);
  return (rgba[0] > 0) && (rgba[1] > 0) && (rgba[2] > 0) && (rgba[3] > 0);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::CreateAWindow()
{
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  // request webgl2 (similar to gles 3.0)
  attrs.majorVersion = 2;
  // Not to be confused with the MultiSamples property on vtkRenderWindow.
  // This `antialias` property concerns the webgl2 context of a canvas.
  // VTK renders to offscreen (in memory) multi-sampled framebuffers and resolves
  // the pixels to the draw buffer when it's time to swap buffers. Consequently,
  // the final blit expects that the destination framebuffer is not multi-sampled.
  // If you turn this on for the canvas's webgl2 context, you will see a black screen and a warning
  // in
  // 1. chrome - [.WebGL-0x9fc004be900] GL_INVALID_OPERATION: Invalid operation on multisampled
  // framebuffer
  // 2. firefox - WebGL warning: blitFramebuffer: DRAW_FRAMEBUFFER may not have multiple samples.
  attrs.antialias = EM_FALSE;
  // optionally blend the canvas with underlying web page contents.
  attrs.alpha = this->EnableTranslucentSurface;
  // when the canvas is translucent (EnableTranslucentSurface is enabled), it's important that
  // premultipliedAlpha is also enabled because VTK results are premultiplied alpha by the default
  // blending function (see vtkOpenGLRenderWindow::Start).
  attrs.premultipliedAlpha = EM_TRUE;
  attrs.depth = EM_TRUE;
  attrs.stencil = this->StencilCapable;

  // choose power preference
  switch (PowerPreference)
  {
    case PowerPreferenceType::Default:
      attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;
      break;
    case PowerPreferenceType::LowPower:
      attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_LOW_POWER;
      break;
    case PowerPreferenceType::HighPerformance:
      attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
      break;
  }

  const auto result = emscripten_webgl_create_context(this->CanvasSelector, &attrs);
  if (result <= 0)
  {
    vtkErrorMacro("Error (" << result << ") initializing WebGL2.");
    return;
  }
  if (emscripten_webgl_make_context_current(result) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    emscripten_webgl_destroy_context(result);
    return;
  }
  this->ContextId = result;

  int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  this->SetSize(width, height);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::Initialize()
{
  if (!this->ContextId)
  {
    this->CreateAWindow();
  }
  if (!this->ContextId)
  {
    vtkErrorMacro("Failed to create Emscripten opengl context");
    return;
  }
  this->OpenGLInit();
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::Finalize()
{
  this->DestroyWindow();
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::DestroyWindow()
{
  this->Clean();
}

//------------------------------------------------------------------------------
int* vtkWebAssemblyOpenGLRenderWindow::GetScreenSize(void)
{
  emscripten_get_screen_size(&this->ScreenSize[0], &this->ScreenSize[1]);
  return this->ScreenSize;
}

//------------------------------------------------------------------------------
int* vtkWebAssemblyOpenGLRenderWindow::GetPosition(void)
{
  return this->Position;
}

namespace
{
//------------------------------------------------------------------------------
EM_BOOL HandleCanvasResize(
  int vtkNotUsed(eventType), const void* vtkNotUsed(reserved), void* userData)
{
  // this is used during fullscreen changes
  auto window = reinterpret_cast<vtkWebAssemblyOpenGLRenderWindow*>(userData);
  int screenSize[2];
  emscripten_get_screen_size(&screenSize[0], &screenSize[1]);
  window->SetSize(screenSize[0], screenSize[1]);
  return 0;
}
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::SetFullScreen(vtkTypeBool arg)
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
  float dpr = emscripten_get_device_pixel_ratio();
  int result = -1;
  if (this->FullScreen)
  {
    EmscriptenFullscreenStrategy strategy;
    strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
    if (dpr != 1.0f)
    {
      strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
    }
    else
    {
      strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
    }

    strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;

    strategy.canvasResizedCallback = ::HandleCanvasResize;
    strategy.canvasResizedCallbackUserData = this;

    result = emscripten_request_fullscreen_strategy(this->CanvasSelector, 1, &strategy);
  }
  else
  {
    result = emscripten_exit_fullscreen();
  }

  if ((result != EMSCRIPTEN_RESULT_SUCCESS) && (result == EMSCRIPTEN_RESULT_DEFERRED))
  {
    vtkErrorMacro(<< "Failed to request fullscreen");
    return;
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ContextId: " << this->ContextId << "\n";
}

namespace
{
void setCursorVisibility(const char* target, bool visible)
{
  // clang-format off
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
    MAIN_THREAD_EM_ASM({findCanvasEventTarget($0).style.cursor = $1 ? 'default' : 'none'; }, target, visible);
#pragma clang diagnostic pop
  // clang-format on
}
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::HideCursor()
{
  ::setCursorVisibility(this->CanvasSelector, false);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyOpenGLRenderWindow::ShowCursor()
{
  ::setCursorVisibility(this->CanvasSelector, true);
}
VTK_ABI_NAMESPACE_END
