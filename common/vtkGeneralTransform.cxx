/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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

#include "vtkGeneralTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGeneralTransform");
  if(ret)
    {
    return (vtkGeneralTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGeneralTransform;
}

//----------------------------------------------------------------------------
vtkGeneralTransform::vtkGeneralTransform()
{
  this->Input = NULL;

  // most of the functionality is provided by the concatenation
  this->Concatenation = vtkTransformConcatenation::New();

  // the stack will be allocated the first time Push is called
  this->Stack = NULL;
}

//----------------------------------------------------------------------------
vtkGeneralTransform::~vtkGeneralTransform()
{
  this->SetInput(NULL);

  if (this->Concatenation)
    {
    this->Concatenation->Delete();
    }
  if (this->Stack)
    {
    this->Stack->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkAbstractTransform::PrintSelf(os,indent);
  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "InverseFlag: " << this->GetInverseFlag() << "\n";
  os << indent << "NumberOfConcatenatedTransforms: " <<
    this->GetNumberOfConcatenatedTransforms() << "\n";
  if (this->GetNumberOfConcatenatedTransforms() != 0)
    {
    int n = this->GetNumberOfConcatenatedTransforms();
    for (int i = 0; i < n; i++)
      {
      vtkAbstractTransform *t = this->GetConcatenatedTransform(i);
      os << indent << "    " << i << ": " << t->GetClassName() << " at " <<
	 t << "\n";
      }
    }
}

//------------------------------------------------------------------------
// Pass the point through each transform in turn
template<class T2, class T3>
static inline void vtkConcatenationTransformPoint(vtkAbstractTransform *input,
					   vtkTransformConcatenation *concat,
					   T2 point[3], T3 output[3])
{
  output[0] = point[0];
  output[1] = point[1];
  output[2] = point[2];

  int i = 0;
  int nTransforms = concat->GetNumberOfTransforms();
  int nPreTransforms = concat->GetNumberOfPreTransforms();
  
  // push point through the PreTransforms
  for (; i < nPreTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformPoint(output,output);
    }    

  // push point though the Input, if present
  if (input)
    {
    if (concat->GetInverseFlag())
      {
      input = input->GetInverse();
      }
    input->InternalTransformPoint(output,output);
    }

  // push point through PostTransforms
  for (; i < nTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformPoint(output,output);
    }
}  

//----------------------------------------------------------------------------
// Pass the point through each transform in turn,
// concatenate the derivatives.
template<class T2, class T3, class T4>
static inline void vtkConcatenationTransformDerivative(
					    vtkAbstractTransform *input,
		                            vtkTransformConcatenation *concat,
		                            T2 point[3], T3 output[3],
                                            T4 derivative[3][3])
{
  T4 matrix[3][3];

  output[0] = point[0];
  output[1] = point[1];
  output[2] = point[2];

  vtkMath::Identity3x3(derivative);

  int i = 0;
  int nTransforms = concat->GetNumberOfTransforms();
  int nPreTransforms = concat->GetNumberOfPreTransforms();
  
  // push point through the PreTransforms
  for (; i < nPreTransforms; i++)
    { 
    concat->GetTransform(i)->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }    

  // push point though the Input, if present
  if (input)
    {
    if (concat->GetInverseFlag())
      {
      input = input->GetInverse();
      }
    input->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }

  // push point through PostTransforms
  for (; i < nTransforms; i++)
    {
    concat->GetTransform(i)->InternalTransformDerivative(output,output,matrix);
    vtkMath::Multiply3x3(matrix,derivative,derivative);
    }
}
  
//------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformPoint(const float input[3],
						 float output[3])
{
  vtkConcatenationTransformPoint(this->Input,this->Concatenation,input,output);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformPoint(const double input[3],
						 double output[3])
{
  vtkConcatenationTransformPoint(this->Input,this->Concatenation,input,output);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformDerivative(const float input[3], 
						      float output[3],
						      float derivative[3][3])
{
  vtkConcatenationTransformDerivative(this->Input,this->Concatenation,
				      input,output,derivative);
}
  
//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalTransformDerivative(const double input[3], 
						      double output[3],
						      double derivative[3][3])
{
  vtkConcatenationTransformDerivative(this->Input,this->Concatenation,
				      input,output,derivative);
}
  
//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalDeepCopy(vtkAbstractTransform *gtrans)
{
  vtkGeneralTransform *transform = 
    (vtkGeneralTransform *)gtrans;

  // copy the input
  this->SetInput(transform->Input);

  // copy the concatenation
  this->Concatenation->DeepCopy(transform->Concatenation);

  // copy the stack
  if (transform->Stack)
    {
    if (this->Stack == NULL)
      {
      this->Stack = vtkTransformConcatenationStack::New();
      }
    this->Stack->DeepCopy(transform->Stack);
    }
  else
    {
    if (this->Stack)
      {
      this->Stack->Delete();
      this->Stack = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::InternalUpdate()
{
  // update the input
  if (this->Input)
    {
    if (this->Concatenation->GetInverseFlag())
      {
      this->Input->GetInverse()->Update();
      }
    else
      {
      this->Input->Update();
      }
    }

  // update the concatenation
  int nTransforms = this->Concatenation->GetNumberOfTransforms();
  for (int i = 0; i < nTransforms; i++)
    {
    this->Concatenation->GetTransform(i)->Update();
    }
}  

//----------------------------------------------------------------------------
void vtkGeneralTransform::Concatenate(vtkAbstractTransform *transform)
{
  if (transform->CircuitCheck(this))
    {
    vtkErrorMacro("Concatenate: this would create a circular reference.");
    return; 
    }
  this->Concatenation->Concatenate(transform); 
  this->Modified(); 
};

//----------------------------------------------------------------------------
void vtkGeneralTransform::SetInput(vtkAbstractTransform *input)
{
  if (this->Input == input) 
    { 
    return; 
    }
  if (input && input->CircuitCheck(this)) 
    {
    vtkErrorMacro("SetInput: this would create a circular reference.");
    return; 
    }
  if (this->Input) 
    { 
    this->Input->Delete(); 
    }
  this->Input = input;
  if (this->Input) 
    { 
    this->Input->Register(this); 
    }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkGeneralTransform::CircuitCheck(vtkAbstractTransform *transform)
{
  if (this->vtkAbstractTransform::CircuitCheck(transform) ||
      this->Input && this->Input->CircuitCheck(transform))
    {
    return 1;
    }

  int n = this->Concatenation->GetNumberOfTransforms();
  for (int i = 0; i < n; i++)
    {
    if (this->Concatenation->GetTransform(i)->CircuitCheck(transform))
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkGeneralTransform::MakeTransform()
{
  return vtkGeneralTransform::New();
}

//----------------------------------------------------------------------------
unsigned long vtkGeneralTransform::GetMTime()
{
  unsigned long mtime = this->vtkAbstractTransform::GetMTime();
  unsigned long mtime2;

  if (this->Input)
    {
    mtime2 = this->Input->GetMTime();
    if (mtime2 > mtime)
      {
      mtime = mtime2;
      }
    }
  mtime2 = this->Concatenation->GetMaxMTime();
  if (mtime2 > mtime)
    {
    return mtime2;
    }
  return mtime;
}



