// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMatrixToLinearTransform.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMatrixToLinearTransform);
vtkCxxSetObjectMacro(vtkMatrixToLinearTransform, Input, vtkMatrix4x4);

//------------------------------------------------------------------------------
vtkMatrixToLinearTransform::vtkMatrixToLinearTransform()
{
  this->Input = nullptr;
  this->InverseFlag = 0;
}

//------------------------------------------------------------------------------
vtkMatrixToLinearTransform::~vtkMatrixToLinearTransform()
{
  this->SetInput(nullptr);
}

//------------------------------------------------------------------------------
void vtkMatrixToLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Update();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
}

//------------------------------------------------------------------------------
void vtkMatrixToLinearTransform::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMatrixToLinearTransform::InternalUpdate()
{
  if (this->Input)
  {
    this->Matrix->DeepCopy(this->Input);
    if (this->InverseFlag)
    {
      this->Matrix->Invert();
    }
  }
  else
  {
    this->Matrix->Identity();
  }
}

//------------------------------------------------------------------------------
void vtkMatrixToLinearTransform::InternalDeepCopy(vtkAbstractTransform* gtrans)
{
  vtkMatrixToLinearTransform* transform = static_cast<vtkMatrixToLinearTransform*>(gtrans);

  this->SetInput(transform->Input);

  if (this->InverseFlag != transform->InverseFlag)
  {
    this->Inverse();
  }
}

//------------------------------------------------------------------------------
vtkAbstractTransform* vtkMatrixToLinearTransform::MakeTransform()
{
  return vtkMatrixToLinearTransform::New();
}

//------------------------------------------------------------------------------
// Get the MTime
vtkMTimeType vtkMatrixToLinearTransform::GetMTime()
{
  vtkMTimeType mtime = this->vtkLinearTransform::GetMTime();

  if (this->Input)
  {
    vtkMTimeType matrixMTime = this->Input->GetMTime();
    if (matrixMTime > mtime)
    {
      return matrixMTime;
    }
  }
  return mtime;
}
VTK_ABI_NAMESPACE_END
