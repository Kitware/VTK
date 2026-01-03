// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebAssemblyHardwareWindow.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebAssemblyHardwareWindow);

//------------------------------------------------------------------------------
vtkWebAssemblyHardwareWindow::vtkWebAssemblyHardwareWindow()
{
  this->SetCanvasSelector("#canvas");
  this->SetStencilCapable(1);

  // set position to -1 to let SDL place the window
  // SetPosition will still work. Defaults of 0,0 result
  // in the window title bar being off screen.
  this->Position[0] = -1;
  this->Position[1] = -1;

  this->WindowId = nullptr;
  this->Platform = "Emscripten";
}

//------------------------------------------------------------------------------
vtkWebAssemblyHardwareWindow::~vtkWebAssemblyHardwareWindow()
{
  this->Destroy();

  // vtkRenderer* renderer;
  // vtkCollectionSimpleIterator rit;
  // this->Renderers->InitTraversal(rit);
  // while ((renderer = this->Renderers->GetNextRenderer(rit)))
  // {
  //   renderer->SetRenderWindow(nullptr);
  // }
  this->SetCanvasSelector(nullptr);
}

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebAssemblyHardwareWindow::CreateOverrideAttributes()
{
  auto* platformAttribute =
    vtkOverrideAttribute::CreateAttributeChain("Platform", "WebAssembly", nullptr);
  auto* windowSystemAttribute =
    vtkOverrideAttribute::CreateAttributeChain("WindowSystem", "HTML5", platformAttribute);
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", windowSystemAttribute);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  os << this->WindowId << '\n';
  this->Superclass::PrintSelf(os, indent);
}

namespace
{
//------------------------------------------------------------------------------------------------
EM_BOOL HandleCanvasResize(
  int vtkNotUsed(eventType), const void* vtkNotUsed(reserved), void* userData)
{
  // this is used during fullscreen changes
  auto window = reinterpret_cast<vtkWebAssemblyHardwareWindow*>(userData);
  int screenSize[2];
  emscripten_get_screen_size(&screenSize[0], &screenSize[1]);
  window->SetSize(screenSize[0], screenSize[1]);
  return 0;
}
}

//------------------------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::SetFullScreen(vtkTypeBool arg)
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

//------------------------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::SetSize(int width, int height)
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

// //------------------------------------------------------------------------------------------------
// int* vtkWebAssemblyHardwareWindow::GetScreenSize()
// {
//   emscripten_get_screen_size(&this->ScreenSize[0], &this->ScreenSize[1]);
//   return this->ScreenSize;
// }

//------------------------------------------------------------------------------------------------
int* vtkWebAssemblyHardwareWindow::GetPosition()
{
  return this->Position;
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

//------------------------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::HideCursor()
{
  ::setCursorVisibility(this->CanvasSelector, false);
}

//------------------------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::ShowCursor()
{
  ::setCursorVisibility(this->CanvasSelector, true);
}

//------------------------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::Create()
{
  int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  this->SetSize(width, height);
  this->WindowId = const_cast<void*>(static_cast<const void*>(this->CanvasSelector));
}

//------------------------------------------------------------------------------------------------
void vtkWebAssemblyHardwareWindow::Destroy()
{
  this->WindowId = nullptr;
}

//------------------------------------------------------------------------------------------------
void* vtkWebAssemblyHardwareWindow::GetGenericWindowId()
{
  return this->WindowId;
}

//------------------------------------------------------------------------------------------------
void* vtkWebAssemblyHardwareWindow::GetGenericDrawable()
{
  return this->WindowId;
}

VTK_ABI_NAMESPACE_END
