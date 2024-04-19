// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInteractorStyleTrackball.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPropPicker.h"
#include "vtkRenderWindowInteractor.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkInteractorStyleTrackball);

//------------------------------------------------------------------------------
vtkInteractorStyleTrackball::vtkInteractorStyleTrackball()
{
  vtkWarningMacro("vtkInteractorStyleTrackball will be deprecated in"
    << endl
    << "the next release after VTK 4.0. Please use" << endl
    << "vtkInteractorStyleSwitch instead.");
}

//------------------------------------------------------------------------------
vtkInteractorStyleTrackball::~vtkInteractorStyleTrackball() = default;

//------------------------------------------------------------------------------
void vtkInteractorStyleTrackball::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
