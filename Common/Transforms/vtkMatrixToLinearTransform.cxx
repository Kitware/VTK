/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixToLinearTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMatrixToLinearTransform.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMatrixToLinearTransform);
vtkCxxSetObjectMacro(vtkMatrixToLinearTransform,Input,vtkMatrix4x4);

//----------------------------------------------------------------------------
vtkMatrixToLinearTransform::vtkMatrixToLinearTransform()
{
  this->Input = NULL;
  this->InverseFlag = 0;
}

//----------------------------------------------------------------------------
vtkMatrixToLinearTransform::~vtkMatrixToLinearTransform()
{
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkMatrixToLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Update();

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
}

//----------------------------------------------------------------------------
void vtkMatrixToLinearTransform::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkMatrixToLinearTransform::InternalDeepCopy(vtkAbstractTransform *gtrans)
{
  vtkMatrixToLinearTransform *transform =
    static_cast<vtkMatrixToLinearTransform *>(gtrans);

  this->SetInput(transform->Input);

  if (this->InverseFlag != transform->InverseFlag)
  {
    this->Inverse();
  }
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkMatrixToLinearTransform::MakeTransform()
{
  return vtkMatrixToLinearTransform::New();
}

//----------------------------------------------------------------------------
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
