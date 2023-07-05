// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkContextMapper2D.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkTable.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkContextMapper2D);
//------------------------------------------------------------------------------
vtkContextMapper2D::vtkContextMapper2D()
{
  // We take 1 input and no outputs
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//------------------------------------------------------------------------------
vtkContextMapper2D::~vtkContextMapper2D() = default;

//------------------------------------------------------------------------------
void vtkContextMapper2D::SetInputData(vtkTable* input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
vtkTable* vtkContextMapper2D::GetInput()
{
  return vtkTable::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//------------------------------------------------------------------------------
int vtkContextMapper2D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
void vtkContextMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
