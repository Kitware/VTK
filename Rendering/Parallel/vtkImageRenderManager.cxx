// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageRenderManager.h"

#include "vtkFloatArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageRenderManager);

//------------------------------------------------------------------------------
vtkImageRenderManager::vtkImageRenderManager() = default;

//------------------------------------------------------------------------------
vtkImageRenderManager::~vtkImageRenderManager() = default;

//------------------------------------------------------------------------------
void vtkImageRenderManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkImageRenderManager::PreRenderProcessing()
{
  // Turn swap buffers off before the render so the end render method has a
  // chance to add to the back buffer.
  if (this->UseBackBuffer)
  {
    this->RenderWindow->SwapBuffersOff();
  }
}

//------------------------------------------------------------------------------
void vtkImageRenderManager::PostRenderProcessing()
{
  if (!this->UseCompositing || this->CheckForAbortComposite())
  {
    return;
  }

  // Swap buffers here
  if (this->UseBackBuffer)
  {
    this->RenderWindow->SwapBuffersOn();
  }
  this->RenderWindow->Frame();
}
VTK_ABI_NAMESPACE_END
