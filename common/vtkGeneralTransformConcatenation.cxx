/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformConcatenation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkGeneralTransformConcatenation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkGeneralTransformConcatenation *vtkGeneralTransformConcatenation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGeneralTransformConcatenation");
  if(ret)
    {
    return (vtkGeneralTransformConcatenation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGeneralTransformConcatenation;
}

//----------------------------------------------------------------------------
vtkGeneralTransformConcatenation::vtkGeneralTransformConcatenation()
{
  this->Concatenation = vtkSimpleTransformConcatenation::New(this);
}

//----------------------------------------------------------------------------
vtkGeneralTransformConcatenation::~vtkGeneralTransformConcatenation()
{
  this->Concatenation->Delete();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGeneralTransform::PrintSelf(os,indent);
  this->Concatenation->PrintSelf(os,indent);
}

//------------------------------------------------------------------------
// Pass the point through each transform in turn
template<class T2, class T3>
static inline void vtkConcatenationTransformPoint(
		 vtkGeneralTransform::vtkSimpleTransformConcatenation *concat,
		 T2 input[3], T3 output[3])
{
  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];

  for (int i = 0; i < concat->GetNumberOfTransforms(); i++)
    {
    concat->GetTransform(i)->InternalTransformPoint(output,output);
    }
}  

//----------------------------------------------------------------------------
// Pass the point through each transform in turn,
// concatenate the derivatives.
template<class T2, class T3, class T4>
static inline void vtkConcatenationTransformDerivative(
		 vtkGeneralTransform::vtkSimpleTransformConcatenation *concat,
		 T2 input[3], T3 output[3], T4 derivative[3][3])
{
  T4 matrix[3][3];

  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];

  vtkMath::Identity3x3(derivative);

  for (int i = 0; i < concat->GetNumberOfTransforms(); i++)
    {
    concat->GetTransform(i)->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }
}
  
//------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::InternalTransformPoint(
					      const float input[3],
					      float output[3])
{
  vtkConcatenationTransformPoint(this->Concatenation,input,output);
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::InternalTransformPoint(
					      const double input[3],
					      double output[3])
{
  vtkConcatenationTransformPoint(this->Concatenation,input,output);
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::InternalTransformDerivative(
						   const float input[3], 
						   float output[3],
						   float derivative[3][3])
{
  vtkConcatenationTransformDerivative(this->Concatenation,
				      input,output,derivative);
}
  
//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::InternalTransformDerivative(
						   const double input[3], 
						   double output[3],
						   double derivative[3][3])
{
  vtkConcatenationTransformDerivative(this->Concatenation,
				      input,output,derivative);
}
  
//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransformConcatenation::MakeTransform()
{
  return vtkGeneralTransformConcatenation::New();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::InternalDeepCopy(
					      vtkGeneralTransform *transform)
{
  vtkGeneralTransformConcatenation *t = 
    (vtkGeneralTransformConcatenation *)transform;

  this->Concatenation->DeepCopy(t->Concatenation);
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Update()
{
  for (int i = 0; i < this->Concatenation->GetNumberOfTransforms(); i++)
    {
    this->Concatenation->GetTransform(i)->Update();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkGeneralTransformConcatenation::GetMTime()
{
  unsigned long mtime1 = this->vtkGeneralTransform::GetMTime();
  unsigned long mtime2 = this->Concatenation->GetMaxMTime();
  return ((mtime1 > mtime2) ? mtime1 : mtime2 );
}



