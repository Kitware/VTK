/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Hide VTK_DEPRECATED_IN_9_1_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkGenericOpenGLRenderWindow.h"

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRendererCollection.h"

vtkStandardNewMacro(vtkGenericOpenGLRenderWindow);

vtkGenericOpenGLRenderWindow::vtkGenericOpenGLRenderWindow()
{
  this->ReadyForRendering = true;
  this->DirectStatus = 0;
  this->CurrentStatus = false;
  this->SupportsOpenGLStatus = 0;
  this->ForceMaximumHardwareLineWidth = 0;
  this->FrameBlitMode = BlitToCurrent;
}

vtkGenericOpenGLRenderWindow::~vtkGenericOpenGLRenderWindow()
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

void vtkGenericOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

float vtkGenericOpenGLRenderWindow::GetMaximumHardwareLineWidth()
{
  return this->ForceMaximumHardwareLineWidth > 0 ? this->ForceMaximumHardwareLineWidth
                                                 : this->Superclass::GetMaximumHardwareLineWidth();
}

void vtkGenericOpenGLRenderWindow::SetFrontLeftBuffer(unsigned int) {}

void vtkGenericOpenGLRenderWindow::SetFrontRightBuffer(unsigned int) {}

void vtkGenericOpenGLRenderWindow::SetBackLeftBuffer(unsigned int) {}

void vtkGenericOpenGLRenderWindow::SetBackRightBuffer(unsigned int) {}

void vtkGenericOpenGLRenderWindow::SetOwnContext(vtkTypeBool val)
{
  this->OwnContext = val;
}

void vtkGenericOpenGLRenderWindow::Finalize()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  this->ReleaseGraphicsResources(this);
}

void vtkGenericOpenGLRenderWindow::Frame()
{
  this->Superclass::Frame();
  this->InvokeEvent(vtkCommand::WindowFrameEvent, nullptr);
}

void vtkGenericOpenGLRenderWindow::MakeCurrent()
{
  this->InvokeEvent(vtkCommand::WindowMakeCurrentEvent, nullptr);
}

bool vtkGenericOpenGLRenderWindow::IsCurrent()
{
  this->InvokeEvent(vtkCommand::WindowIsCurrentEvent, &this->CurrentStatus);
  return this->CurrentStatus;
}

int vtkGenericOpenGLRenderWindow::SupportsOpenGL()
{
  this->InvokeEvent(vtkCommand::WindowSupportsOpenGLEvent, &this->SupportsOpenGLStatus);
  return this->SupportsOpenGLStatus;
}

vtkTypeBool vtkGenericOpenGLRenderWindow::IsDirect()
{
  this->InvokeEvent(vtkCommand::WindowIsDirectEvent, &this->DirectStatus);
  return this->DirectStatus;
}

void vtkGenericOpenGLRenderWindow::SetWindowId(void*) {}

void* vtkGenericOpenGLRenderWindow::GetGenericWindowId()
{
  return nullptr;
}

void vtkGenericOpenGLRenderWindow::SetDisplayId(void*) {}

void vtkGenericOpenGLRenderWindow::SetParentId(void*) {}

void* vtkGenericOpenGLRenderWindow::GetGenericDisplayId()
{
  return nullptr;
}

void* vtkGenericOpenGLRenderWindow::GetGenericParentId()
{
  return nullptr;
}

void* vtkGenericOpenGLRenderWindow::GetGenericContext()
{
  return nullptr;
}

void* vtkGenericOpenGLRenderWindow::GetGenericDrawable()
{
  return nullptr;
}

void vtkGenericOpenGLRenderWindow::SetWindowInfo(const char*) {}

void vtkGenericOpenGLRenderWindow::SetParentInfo(const char*) {}

int* vtkGenericOpenGLRenderWindow::GetScreenSize()
{
  return this->ScreenSize;
}

void vtkGenericOpenGLRenderWindow::HideCursor() {}

void vtkGenericOpenGLRenderWindow::ShowCursor() {}

void vtkGenericOpenGLRenderWindow::SetFullScreen(vtkTypeBool) {}

void vtkGenericOpenGLRenderWindow::WindowRemap() {}

vtkTypeBool vtkGenericOpenGLRenderWindow::GetEventPending()
{
  return 0;
}

void vtkGenericOpenGLRenderWindow::SetNextWindowId(void*) {}

void vtkGenericOpenGLRenderWindow::SetNextWindowInfo(const char*) {}

void vtkGenericOpenGLRenderWindow::CreateAWindow() {}

void vtkGenericOpenGLRenderWindow::DestroyWindow() {}

void vtkGenericOpenGLRenderWindow::SetIsDirect(vtkTypeBool newValue)
{
  this->DirectStatus = newValue;
}

void vtkGenericOpenGLRenderWindow::SetSupportsOpenGL(int newValue)
{
  this->SupportsOpenGLStatus = newValue;
}

void vtkGenericOpenGLRenderWindow::SetIsCurrent(bool newValue)
{
  this->CurrentStatus = newValue;
}

void vtkGenericOpenGLRenderWindow::OpenGLInit()
{
  auto state = this->GetState();
  state->Reset();
  state->Push();
  this->Superclass::OpenGLInit();
  state->Pop();
}

void vtkGenericOpenGLRenderWindow::Render()
{
  if (this->ReadyForRendering)
  {
    vtkOpenGLCheckErrorMacro("error before running VTK rendering code");
    if (!this->Initialized)
    {
      this->OpenGLInit();
    }
    this->MakeCurrent();
    if (!this->IsCurrent())
    {
      vtkLogF(TRACE, "rendering skipped since `MakeCurrent` was not successful.");
    }
    else
    {
      // Query current GL state and store them
      auto state = this->GetState();
      state->Reset();
      state->Push();

      this->Superclass::Render();

      // Restore state to previous known value
      state->Pop();
    }
    vtkOpenGLCheckErrorMacro("error after running VTK rendering code");
  }
}

void vtkGenericOpenGLRenderWindow::SetCurrentCursor(int cShape)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting current Cursor to "
                << cShape);
  if (this->GetCurrentCursor() != cShape)
  {
    this->CurrentCursor = cShape;
    this->Modified();
    this->InvokeEvent(vtkCommand::CursorChangedEvent, &cShape);
  }
}

int vtkGenericOpenGLRenderWindow::ReadPixels(
  const vtkRecti& rect, int front, int glFormat, int glType, void* data, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::ReadPixels(rect, front, glFormat, glType, data, right);
  }

  vtkWarningMacro("`ReadPixels` called before window is ready for rendering; ignoring.");
  return VTK_ERROR;
}

int vtkGenericOpenGLRenderWindow::SetPixelData(
  int x1, int y1, int x2, int y2, unsigned char* data, int front, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetPixelData(x1, y1, x2, y2, data, front, right);
  }

  vtkWarningMacro("`SetPixelData` called before window is ready for rendering; ignoring.");
  return VTK_ERROR;
}

int vtkGenericOpenGLRenderWindow::SetPixelData(
  int x1, int y1, int x2, int y2, vtkUnsignedCharArray* data, int front, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetPixelData(x1, y1, x2, y2, data, front, right);
  }

  vtkWarningMacro("`SetPixelData` called before window is ready for rendering; ignoring.");
  return VTK_ERROR;
}

int vtkGenericOpenGLRenderWindow::SetRGBACharPixelData(
  int x1, int y1, int x2, int y2, unsigned char* data, int front, int blend, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetRGBACharPixelData(x1, y1, x2, y2, data, front, blend, right);
  }

  vtkWarningMacro("`SetRGBACharPixelData` called before window is ready for rendering; ignoring.");
  return VTK_ERROR;
}

int vtkGenericOpenGLRenderWindow::SetRGBACharPixelData(
  int x1, int y1, int x2, int y2, vtkUnsignedCharArray* data, int front, int blend, int right)
{
  if (this->ReadyForRendering)
  {
    this->MakeCurrent();
    this->GetState()->ResetFramebufferBindings();
    return this->Superclass::SetRGBACharPixelData(x1, y1, x2, y2, data, front, blend, right);
  }

  vtkWarningMacro("`SetRGBACharPixelData` called before window is ready for rendering; ignoring.");
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
bool vtkGenericOpenGLRenderWindow::IsDrawable()
{
  VTK_LEGACY_BODY(vtkGenericOpenGLRenderWindow::IsDrawable, "VTK 9.1");

  return this->ReadyForRendering;
}
