// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTrivialConsumer.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTrivialConsumer);

//------------------------------------------------------------------------------
vtkTrivialConsumer::vtkTrivialConsumer()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//------------------------------------------------------------------------------
vtkTrivialConsumer::~vtkTrivialConsumer() = default;

//------------------------------------------------------------------------------
void vtkTrivialConsumer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkTrivialConsumer::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkTrivialConsumer::FillOutputPortInformation(int, vtkInformation*)
{
  return 1;
}
VTK_ABI_NAMESPACE_END
