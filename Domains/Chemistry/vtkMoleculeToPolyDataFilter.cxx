// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMoleculeToPolyDataFilter.h"

#include "vtkInformation.h"
#include "vtkMolecule.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkMoleculeToPolyDataFilter::vtkMoleculeToPolyDataFilter()
{
  this->SetNumberOfInputPorts(1);
}

//------------------------------------------------------------------------------
vtkMoleculeToPolyDataFilter::~vtkMoleculeToPolyDataFilter() = default;

//------------------------------------------------------------------------------
vtkMolecule* vtkMoleculeToPolyDataFilter::GetInput()
{
  return vtkMolecule::SafeDownCast(this->Superclass::GetInput(0));
}

//------------------------------------------------------------------------------
int vtkMoleculeToPolyDataFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");
  return 1;
}

//------------------------------------------------------------------------------
void vtkMoleculeToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
