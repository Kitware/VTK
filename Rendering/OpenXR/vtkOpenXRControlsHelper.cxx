// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRControlsHelper.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRControlsHelper);

//------------------------------------------------------------------------------
void vtkOpenXRControlsHelper::InitControlPosition()
{
  // Set this->ControlPositionLC to position tooltip on component.
  //     https://gitlab.kitware.com/vtk/vtk/-/issues/18332
  // See implementation in vtkOpenVRControlsHelper for example/starting point
}

//------------------------------------------------------------------------------
void vtkOpenXRControlsHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
