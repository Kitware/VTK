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
vtkMatrixToPerspectiveTransform::vtkMatrixToPerspectiveTransform()
{
  this->Input = NULL;
  this->InverseFlag = 0;
}

//----------------------------------------------------------------------------
vtkMatrixToPerspectiveTransform::~vtkMatrixToPerspectiveTransform()
{
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkMatrixToPerspectiveTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Update();

  vtkPerspectiveTransform::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
}

//----------------------------------------------------------------------------
void vtkMatrixToPerspectiveTransform::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMatrixToPerspectiveTransform::InternalUpdate()
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
void vtkMatrixToPerspectiveTransform::InternalDeepCopy(
						vtkGeneralTransform *gtrans)
{
  vtkMatrixToPerspectiveTransform *transform = 
    (vtkMatrixToPerspectiveTransform *)gtrans;

  this->SetInput(transform->Input);

  if (this->InverseFlag != transform->InverseFlag)
    {
    this->Inverse();
    }
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkMatrixToPerspectiveTransform::MakeTransform()
{
  return vtkMatrixToPerspectiveTransform::New();
}

//----------------------------------------------------------------------------
// Get the MTime
unsigned long vtkMatrixToPerspectiveTransform::GetMTime()
{
  unsigned long mtime = this->vtkPerspectiveTransform::GetMTime();

  if (this->Input)
    {
    unsigned long matrixMTime = this->Input->GetMTime();
    if (matrixMTime > mtime)
      {
      return matrixMTime;
      }
    }
  return mtime;
}
