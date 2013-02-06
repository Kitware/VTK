/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssemblyNode.h"
#include "vtkProp.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkAssemblyNode);

//----------------------------------------------------------------------------
vtkAssemblyNode::vtkAssemblyNode()
{
  this->ViewProp = 0;
  this->Matrix = 0;
}

//----------------------------------------------------------------------------
vtkAssemblyNode::~vtkAssemblyNode()
{
  if (this->Matrix)
    {
    this->Matrix->Delete();
    this->Matrix = 0;
    }
}

//----------------------------------------------------------------------------
// Don't do reference counting
void vtkAssemblyNode::SetViewProp(vtkProp *prop)
{
  this->ViewProp = prop;
}

//----------------------------------------------------------------------------
void vtkAssemblyNode::SetMatrix(vtkMatrix4x4 *matrix)
{
  // delete previous
  if (this->Matrix)
    {
    this->Matrix->Delete();
    this->Matrix = 0;
    }
  // return if NULL matrix specified
  if (!matrix)
    {
    return;
    }

  // else create a copy of the matrix
  vtkMatrix4x4 *newMatrix = vtkMatrix4x4::New();
  newMatrix->DeepCopy(matrix);
  this->Matrix = newMatrix;
}

//----------------------------------------------------------------------------
unsigned long vtkAssemblyNode::GetMTime()
{
  unsigned long propMTime = 0;
  unsigned long matrixMTime = 0;

  if (this->ViewProp)
    {
    propMTime = this->ViewProp->GetMTime();
    }
  if (this->Matrix)
    {
    matrixMTime = this->Matrix->GetMTime();
    }

  return (propMTime > matrixMTime ? propMTime : matrixMTime);
}

//----------------------------------------------------------------------------
void vtkAssemblyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->ViewProp )
    {
    os << indent << "ViewProp: " << this->ViewProp << "\n";
    }
  else
    {
    os << indent << "ViewProp: (none)\n";
    }

  if ( this->Matrix )
    {
    os << indent << "Matrix: " << this->Matrix << "\n";
    }
  else
    {
    os << indent << "Matrix: (none)\n";
    }
}
