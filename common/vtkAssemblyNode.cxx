/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyNode.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkAssemblyNode.h"
#include "vtkProp.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkAssemblyNode* vtkAssemblyNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAssemblyNode");
  if(ret)
    {
    return (vtkAssemblyNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAssemblyNode;
}

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

// Don't do reference counting
void vtkAssemblyNode::SetProp(vtkProp *prop)
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

void vtkAssemblyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

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


