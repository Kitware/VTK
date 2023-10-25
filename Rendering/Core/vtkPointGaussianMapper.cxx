// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointGaussianMapper.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkPointGaussianMapper);

vtkCxxSetObjectMacro(vtkPointGaussianMapper, ScaleFunction, vtkPiecewiseFunction);
vtkCxxSetObjectMacro(vtkPointGaussianMapper, ScalarOpacityFunction, vtkPiecewiseFunction);

//------------------------------------------------------------------------------
vtkPointGaussianMapper::vtkPointGaussianMapper()
{
  this->ScaleArray = nullptr;
  this->ScaleArrayComponent = 0;
  this->OpacityArray = nullptr;
  this->OpacityArrayComponent = 0;
  this->SplatShaderCode = nullptr;

  this->ScaleFunction = nullptr;
  this->ScaleTableSize = 1024;

  this->ScalarOpacityFunction = nullptr;
  this->OpacityTableSize = 1024;

  this->ScaleFactor = 1.0;
  this->Emissive = 1;
  this->BoundScale = 3.0;
}

//------------------------------------------------------------------------------
vtkPointGaussianMapper::~vtkPointGaussianMapper()
{
  this->SetScaleArray(nullptr);
  this->SetOpacityArray(nullptr);
  this->SetSplatShaderCode(nullptr);
  this->SetScalarOpacityFunction(nullptr);
  this->SetScaleFunction(nullptr);
}

//------------------------------------------------------------------------------
void vtkPointGaussianMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Array: " << (this->ScaleArray ? this->ScaleArray : "(none)") << "\n";
  os << indent << "Scale Array Component: " << this->ScaleArrayComponent << "\n";
  os << indent << "Opacity Array: " << (this->OpacityArray ? this->OpacityArray : "(none)") << "\n";
  os << indent << "Opacity Array Component: " << this->OpacityArrayComponent << "\n";
  os << indent << "SplatShaderCode: " << (this->SplatShaderCode ? this->SplatShaderCode : "(none)")
     << "\n";
  os << indent << "ScaleFactor: " << this->ScaleFactor << "\n";
  os << indent << "Emissive: " << this->Emissive << "\n";
  os << indent << "OpacityTableSize: " << this->OpacityTableSize << "\n";
  os << indent << "ScaleTableSize: " << this->ScaleTableSize << "\n";
  os << indent << "BoundScale: " << this->BoundScale << "\n";
}
VTK_ABI_NAMESPACE_END
