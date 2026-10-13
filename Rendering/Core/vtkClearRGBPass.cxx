// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkClearRGBPass.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkClearRGBPass);

//------------------------------------------------------------------------------
vtkClearRGBPass::vtkClearRGBPass()
{
  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;
}

//------------------------------------------------------------------------------
vtkClearRGBPass::~vtkClearRGBPass() = default;

//------------------------------------------------------------------------------
void vtkClearRGBPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Background:" << this->Background[0] << "," << this->Background[1] << ","
     << this->Background[2] << endl;
}
VTK_ABI_NAMESPACE_END
