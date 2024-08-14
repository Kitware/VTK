// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebAssemblyWebGPURenderWindow.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkWebGPUConfiguration.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebAssemblyWebGPURenderWindow);

//------------------------------------------------------------------------------
vtkWebAssemblyWebGPURenderWindow::vtkWebAssemblyWebGPURenderWindow()
{
  this->SetCanvasId("#canvas");
  this->SetStencilCapable(1);

  // set position to -1 to let SDL place the window
  // SetPosition will still work. Defaults of 0,0 result
  // in the window title bar being off screen.
  this->Position[0] = -1;
  this->Position[1] = -1;
}

//------------------------------------------------------------------------------
vtkWebAssemblyWebGPURenderWindow::~vtkWebAssemblyWebGPURenderWindow()
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
void vtkWebAssemblyWebGPURenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  os << this->WindowId << '\n';
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------------------------
std::string vtkWebAssemblyWebGPURenderWindow::MakeDefaultWindowNameWithBackend()
{
  if (this->WGPUConfiguration)
  {
    return std::string("Visualization Toolkit - ") + "Emscripten " +
      this->WGPUConfiguration->GetBackendInUseAsString();
  }
  else
  {
    return "Visualization Toolkit - Emscripten undefined backend";
  }
}

//------------------------------------------------------------------------------
bool vtkWebAssemblyWebGPURenderWindow::WindowSetup()
{
  vtkDebugMacro(<< __func__);
  if (!this->WGPUConfiguration)
  {
    vtkErrorMacro(
      << "vtkWebGPUConfiguration is null! Please provide one with SetWGPUConfiguration");
    return false;
  }
  if (!this->WindowId)
  {
    this->CreateAWindow();
  }
  if (this->WGPUInit())
  {
    // render into canvas elememnt
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector htmlSurfDesc;
    htmlSurfDesc.selector = "#canvas";
    wgpu::SurfaceDescriptor surfDesc = {};
    surfDesc.label = "VTK HTML5 surface";
    surfDesc.nextInChain = &htmlSurfDesc;
    this->Surface = this->WGPUConfiguration->GetInstance().CreateSurface(&surfDesc);
  }
  return this->Surface != nullptr;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::Finalize()
{
  if (this->Initialized)
  {
    this->WGPUFinalize();
  }
  this->DestroyWindow();
}

namespace
{
//------------------------------------------------------------------------------
EM_BOOL HandleCanvasResize(
  int vtkNotUsed(eventType), const void* vtkNotUsed(reserved), void* userData)
{
  // this is used during fullscreen changes
  auto window = reinterpret_cast<vtkWebAssemblyWebGPURenderWindow*>(userData);
  int screenSize[2];
  emscripten_get_screen_size(&screenSize[0], &screenSize[1]);
  window->SetSize(screenSize[0], screenSize[1]);
  return 0;
}
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::SetFullScreen(vtkTypeBool arg)
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

    result = emscripten_request_fullscreen_strategy(this->CanvasId, 1, &strategy);
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
void vtkWebAssemblyWebGPURenderWindow::SetShowWindow(bool val)
{
  this->Superclass::SetShowWindow(val);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Size[0] = width;
    this->Size[1] = height;
    emscripten_set_canvas_element_size(this->CanvasId, this->Size[0], this->Size[1]);
    if (this->Interactor)
    {
      this->Interactor->SetSize(this->Size[0], this->Size[1]);
    }
    this->Modified();
    this->InvokeEvent(vtkCommand::WindowResizeEvent, nullptr);
  }
}

//------------------------------------------------------------------------------
int* vtkWebAssemblyWebGPURenderWindow::GetScreenSize()
{
  emscripten_get_screen_size(&this->ScreenSize[0], &this->ScreenSize[1]);
  return this->ScreenSize;
}

//------------------------------------------------------------------------------
int* vtkWebAssemblyWebGPURenderWindow::GetPosition()
{
  return this->Position;
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::SetWindowName(const char* title)
{
  this->Superclass::SetWindowName(title);
  emscripten_set_window_title(title);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::Clean()
{
  this->CleanUpRenderers();
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::Frame()
{
  if (!this->AbortRender)
  {
    this->Superclass::Frame();
  }
}

//------------------------------------------------------------------------------
int vtkWebAssemblyWebGPURenderWindow::GetColorBufferSizes(int* rgba)
{
  if (rgba == nullptr)
  {
    return 0;
  }
  rgba[0] = 8;
  rgba[1] = 8;
  rgba[2] = 8;
  rgba[3] = 8;
  return (rgba[0] > 0) && (rgba[1] > 0) && (rgba[2] > 0) && (rgba[3] > 0);
}

namespace
{
void setCursorVisibility(bool visible)
{
  // clang-format off
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
    MAIN_THREAD_EM_ASM({if (Module['canvas']) { Module['canvas'].style['cursor'] = $0 ? 'default' : 'none'; }}, visible);
#pragma clang diagnostic pop
  // clang-format on
}
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::HideCursor()
{
  ::setCursorVisibility(false);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::ShowCursor()
{
  ::setCursorVisibility(true);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::CleanUpRenderers()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  this->ReleaseGraphicsResources(this);
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::CreateAWindow()
{
  int height = ((this->Size[1] > 0) ? this->Size[1] : 300);
  int width = ((this->Size[0] > 0) ? this->Size[0] : 300);
  this->SetSize(width, height);
  this->WindowId = const_cast<void*>(static_cast<const void*>(this->CanvasId));
}

//------------------------------------------------------------------------------
void vtkWebAssemblyWebGPURenderWindow::DestroyWindow()
{
  this->WindowId = nullptr;
  this->Clean();
}

VTK_ABI_NAMESPACE_END
