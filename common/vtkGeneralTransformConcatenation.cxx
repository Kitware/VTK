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
#include "vtkGeneralTransformInverse.h"
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
  this->InverseFlag = 0;

  this->PreMultiplyFlag = 1;

  this->NumberOfTransforms = 0;
  this->MaxNumberOfTransforms = 0;
  this->TransformList = NULL;
  this->InverseList = NULL;
}

//----------------------------------------------------------------------------
vtkGeneralTransformConcatenation::~vtkGeneralTransformConcatenation()
{
  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Delete();
      this->InverseList[i]->Delete();
      }
    }
  if (this->TransformList)
    {
    delete [] this->TransformList;
    }
  if (this->InverseList)
    {
    delete [] this->InverseList;
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGeneralTransform::PrintSelf(os,indent);

  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
  os << indent << "PreMultiplyFlag: " << this->PreMultiplyFlag << "\n";
  os << indent << "NumberOfTransforms: " << this->NumberOfTransforms << "\n";
  os << indent << "TransformList:\n";

  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    this->TransformList[i]->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Concatenate(vtkGeneralTransform *trans)
{
  if (trans == this)
    {
    vtkErrorMacro(<< "Concatenate: Can't concatenate with self!");
    return;
    }

  vtkGeneralTransform **transList = this->TransformList;
  vtkGeneralTransform **inverseList = this->InverseList;
  int n = this->NumberOfTransforms;
  this->NumberOfTransforms++;
  
  // check to see if we need to allocate more space
  if (this->NumberOfTransforms > this->MaxNumberOfTransforms)
    {
    int nMax = this->MaxNumberOfTransforms + 20;
    transList = new vtkGeneralTransform *[nMax];
    inverseList = new vtkGeneralTransform *[nMax];
    for (int i = 0; i < n; i++)
      {
      transList[i] = this->TransformList[i];
      inverseList[i] = this->InverseList[i];
      }
    if (this->TransformList)
      {
      delete [] this->TransformList;
      }
    if (this->InverseList)
      {
      delete [] this->InverseList;
      }
    this->TransformList = transList;
    this->InverseList = inverseList;
    this->MaxNumberOfTransforms = nMax;
    }

  // add the transform either the beginning or end of the list,
  // according to flags
  if (this->PreMultiplyFlag ^ this->InverseFlag)
    {
    for (int i = n; i > 0; i--)
      {
      transList[i] = transList[i-1];
      inverseList[i] = inverseList[i-1];
      }
    n = 0;
    }

  if (this->InverseFlag)
    {
    trans = trans->GetInverse();
    }

  transList[n] = trans;
  transList[n]->Register(this);
  inverseList[n] = trans->GetInverse();
  inverseList[n]->Register(this);
  
  this->Modified();
}

//----------------------------------------------------------------------------
// concatenate a set of transforms in order.
void vtkGeneralTransformConcatenation::Concatenate(vtkGeneralTransform *t1,
						   vtkGeneralTransform *t2,
						   vtkGeneralTransform *t3,
						   vtkGeneralTransform *t4)
{
  if (this->PreMultiplyFlag)
    {
    this->Concatenate(t1); 
    this->Concatenate(t2);
    if (t3) { this->Concatenate(t3); }
    if (t4) { this->Concatenate(t4); }
    }
  else
    {
    if (t4) { this->Concatenate(t4); }
    if (t3) { this->Concatenate(t3); }
    this->Concatenate(t2);
    this->Concatenate(t1);
    }
}

//------------------------------------------------------------------------
// Check the InverseFlag, and perform a forward or reverse transform
// as appropriate.
void vtkGeneralTransformConcatenation::InternalTransformPoint(
					      const float input[3],
					      float output[3])
{
  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];

  if (this->InverseFlag)
    {
    for (int i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseList[i]->InternalTransformPoint(output,output);
      }
    }
  else
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->InternalTransformPoint(output,output);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::InternalTransformDerivative(
						   const float input[3], 
						   float output[3],
						   float derivative[3][3])
{
  float matrix[3][3];

  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];

  vtkGeneralTransform::Identity3x3(derivative);

  if (this->InverseFlag)
    {
    for (int i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseList[i]->InternalTransformDerivative(output,output,
							matrix);
      vtkGeneralTransform::Multiply3x3(matrix,derivative,derivative);
      }
    }
  else
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->InternalTransformDerivative(output,output,
							  matrix);
      vtkGeneralTransform::Multiply3x3(matrix,derivative,derivative);
      }
    }
}
  
//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Identity()
{
  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Delete();
      this->InverseList[i]->Delete();
      }
    }
  this->NumberOfTransforms = 0;
  this->InverseFlag = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransformConcatenation::MakeTransform()
{
  return vtkGeneralTransformConcatenation::New();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::DeepCopy(vtkGeneralTransform *transform)
{
  if (strcmp("vtkGeneralTransformInverse",transform->GetClassName()) == 0)
    {
    transform = ((vtkGeneralTransformInverse *)transform)->GetTransform();
    }
  if (strcmp("vtkGeneralTransformConcatenation",transform->GetClassName()) 
      != 0)
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    return;
    }

  vtkGeneralTransformConcatenation *t = 
    (vtkGeneralTransformConcatenation *)transform;

  if (t == this)
    {
    return;
    }

  this->PreMultiplyFlag = t->PreMultiplyFlag;
  this->InverseFlag = t->InverseFlag;

  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Delete();
      this->InverseList[i]->Delete();
      }
    }
  if (this->TransformList)
    {
    delete [] this->TransformList;
    }
  if (this->InverseList)
    {
    delete [] this->InverseList;
    }

  this->MaxNumberOfTransforms = t->MaxNumberOfTransforms;
  this->NumberOfTransforms = t->NumberOfTransforms;

  this->TransformList = 
    new vtkGeneralTransform *[this->MaxNumberOfTransforms];
  this->InverseList = 
    new vtkGeneralTransform *[this->MaxNumberOfTransforms];

  // copy the transforms by reference
  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    this->TransformList[i] = t->TransformList[i];
    this->TransformList[i]->Register(this);  
    this->InverseList[i] = t->InverseList[i];
    this->InverseList[i]->Register(this);  
    }  
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Update()
{
  if (this->InverseFlag)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->InverseList[i]->Update();
      }
    }
  else
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Update();
      }
    }
}

//----------------------------------------------------------------------------
unsigned long vtkGeneralTransformConcatenation::GetMTime()
{
  unsigned long result = this->vtkGeneralTransform::GetMTime();

  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    unsigned long mtime = this->TransformList[i]->GetMTime();
    if (mtime > result)
      {
      result = mtime;
      }
    }

  return result;
}

