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

vtkCxxRevisionMacro(vtkAssemblyNode, "1.7");
vtkStandardNewMacro(vtkAssemblyNode);

vtkAssemblyNode::vtkAssemblyNode()
{
  this->Prop = NULL;
  this->Matrix = NULL;
}

vtkAssemblyNode::~vtkAssemblyNode()
{
//  if ( this->Prop )
//    {
//    this->Prop->Delete();
//    }
  if ( this->Matrix )
    {
    this->Matrix->Delete();
    }
}

//----------------------------------------------------------------------------
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef SetProp
// Define possible mangled names.
void vtkAssemblyNode::SetPropA(vtkProp* prop)
{
  this->SetPropInternal(prop);
}
void vtkAssemblyNode::SetPropW(vtkProp* prop)
{
  this->SetPropInternal(prop);
}
#endif
void vtkAssemblyNode::SetProp(vtkProp* prop)
{
  this->SetPropInternal(prop);
}

//----------------------------------------------------------------------------
// Don't do reference counting
void vtkAssemblyNode::SetPropInternal(vtkProp *prop)
{
  this->Prop = prop;
}


void vtkAssemblyNode::SetMatrix(vtkMatrix4x4 *matrix)
{
  // delete previous
  if ( this->Matrix != NULL )
    {
    this->Matrix->Delete();
    this->Matrix = NULL;
    }
  // return if NULL matrix specified
  if ( matrix == NULL )
    {
    return;
    }

  // else create a copy of the matrix
  vtkMatrix4x4 *newMatrix = vtkMatrix4x4::New();
  newMatrix->DeepCopy(matrix);
  this->Matrix = newMatrix;
}

unsigned long vtkAssemblyNode::GetMTime()
{
  unsigned long propMTime=0;
  unsigned long matrixMTime=0;
  
  if ( this->Prop != NULL )
    {
    propMTime = this->Prop->GetMTime();
    }
  if ( this->Matrix != NULL )
    {
    matrixMTime = this->Matrix->GetMTime();
    }
  
  return (propMTime > matrixMTime ? propMTime : matrixMTime);
}

//----------------------------------------------------------------------------
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetProp
// Define possible mangled names.
vtkProp* vtkAssemblyNode::GetPropA()
{
  return this->GetPropInternal();
}
vtkProp* vtkAssemblyNode::GetPropW()
{
  return this->GetPropInternal();
}
#endif
vtkProp* vtkAssemblyNode::GetProp()
{
  return this->GetPropInternal();
}

//----------------------------------------------------------------------------
vtkProp* vtkAssemblyNode::GetPropInternal()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning Prop address " << this->Prop );
  return this->Prop;
}

void vtkAssemblyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Prop )
    {
    os << indent << "Prop: " << this->Prop << "\n";
    }
  else
    {
    os << indent << "Prop: (none)\n";
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


