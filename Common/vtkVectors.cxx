/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectors.cxx
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
#include "vtkVectors.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkVectors* vtkVectors::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVectors");
  if(ret)
    {
    return (vtkVectors*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVectors;
}




vtkVectors *vtkVectors::New(int dataType)
{
  vtkVectors *res = vtkVectors::New();
  res->SetDataType(dataType);
  res->GetData()->SetNumberOfComponents(3);
  return res;
}

// Construct object with an initial data array of type float.
vtkVectors::vtkVectors() 
{
  this->MaxNorm = 0.0;
  this->Data->SetNumberOfComponents(3);
}

// Given a list of pt ids, return an array of vectors.
void vtkVectors::GetVectors(vtkIdList *ptIds, vtkVectors *v)
{
  float vector[3];
  vtkIdType num=ptIds->GetNumberOfIds();
  
  v->SetNumberOfVectors(num);
  for (vtkIdType i=0; i<num; i++)
    {
    this->GetVector(ptIds->GetId(i),vector);
    v->SetVector(i,vector);
    }
}

// Compute the largest norm for these vectors.
void vtkVectors::ComputeMaxNorm()
{
  vtkIdType i;
  float *v, norm;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->MaxNorm = 0.0;
    for (i=0; i<this->GetNumberOfVectors(); i++)
      {
      v = this->GetVector(i);
      norm = vtkMath::Norm(v);
      if ( norm > this->MaxNorm )
	{
	this->MaxNorm = norm;
	}
      }

    this->ComputeTime.Modified();
    }
}

// Return the maximum norm for these vectors.
double vtkVectors::GetMaxNorm()
{
  this->ComputeMaxNorm();
  return this->MaxNorm;
}

void vtkVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkAttributeData::PrintSelf(os,indent);

  os << indent << "Number Of Vectors: " << this->GetNumberOfVectors() << "\n";
  os << indent << "Maximum Euclidean Norm: " << this->GetMaxNorm() << "\n";
}

