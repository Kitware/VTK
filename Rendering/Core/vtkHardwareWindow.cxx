// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHardwareWindow.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkHardwareWindow);

vtkHardwareWindow::vtkHardwareWindow()
{
#ifdef VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
  this->ShowWindow = false;
  this->UseOffScreenBuffers = true;
#else
  this->ShowWindow = true;
#endif
}

//------------------------------------------------------------------------------
vtkHardwareWindow::~vtkHardwareWindow() = default;

//------------------------------------------------------------------------------
void vtkHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkHardwareWindow::SetCoverable(vtkTypeBool coverable)
{
  if (coverable)
  {
    vtkWarningMacro(<< "SetCoverable(" << coverable << ") is unsupported for "
                    << this->GetClassName());
  }
}

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_END
