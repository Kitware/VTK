/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixToPerspectiveTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS EVEN, SOFTWARE IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMatrixToPerspectiveTransform.h"
#include "vtkPerspectiveTransformInverse.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkMatrixToPerspectiveTransform* vtkMatrixToPerspectiveTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMatrixToPerspectiveTransform");
  if(ret)
    {
    return (vtkMatrixToPerspectiveTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMatrixToPerspectiveTransform;
}

//----------------------------------------------------------------------------
void vtkMatrixToPerspectiveTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPerspectiveTransform::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkMatrixToPerspectiveTransform::MakeTransform()
{
  return vtkMatrixToPerspectiveTransform::New();
}

//----------------------------------------------------------------------------
void vtkMatrixToPerspectiveTransform::DeepCopy(vtkGeneralTransform *transform)
{
  if (strcmp("vtkPerspectiveTransformInverse",transform->GetClassName())==0)
    {
    transform = ((vtkPerspectiveTransformInverse *)transform)->GetTransform();
    }
  if (strcmp("vtkMatrixToPerspectiveTransform",transform->GetClassName()) != 0)
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    return;
    }

  vtkMatrixToPerspectiveTransform *t = 
    (vtkMatrixToPerspectiveTransform *)transform;  

  if (t == this)
    {
    return;
    }

  this->Matrix->DeepCopy(t->Matrix);
  this->Modified();
}

//----------------------------------------------------------------------------
// Set the current matrix directly.
void vtkMatrixToPerspectiveTransform::SetMatrix(vtkMatrix4x4 *m)
{
  if (this->Matrix == m)
    {
    return;
    }

  if (this->Matrix)
    {
    this->Matrix->Delete();
    }
  m->Register(this);
  this->Matrix = m;
  this->Modified();
}

//----------------------------------------------------------------------------
// Creates an identity matrix.
void vtkMatrixToPerspectiveTransform::Identity()
{
  this->Matrix->Identity();
  this->Modified();
}

//----------------------------------------------------------------------------
// Inverts the matrix.
void vtkMatrixToPerspectiveTransform::Inverse()
{
  this->Matrix->Invert();
  this->Modified();
}

//----------------------------------------------------------------------------
// Get the MTime
unsigned long vtkMatrixToPerspectiveTransform::GetMTime()
{
  unsigned long mtime = this->vtkPerspectiveTransform::GetMTime();
  unsigned long matrixMTime = this->Matrix->GetMTime();

  if (matrixMTime > mtime)
    {
    mtime = matrixMTime;
    }
  return mtime;
}
