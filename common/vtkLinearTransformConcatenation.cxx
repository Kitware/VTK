/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearTransformConcatenation.cxx
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

#include "vtkLinearTransformConcatenation.h"
#include "vtkLinearTransformInverse.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkLinearTransformConcatenation *vtkLinearTransformConcatenation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLinearTransformConcatenation");
  if(ret)
    {
    return (vtkLinearTransformConcatenation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLinearTransformConcatenation;
}

//----------------------------------------------------------------------------
vtkLinearTransformConcatenation::vtkLinearTransformConcatenation()
{
  this->TransformType = VTK_CONCATENATION_TRANSFORM | VTK_LINEAR_TRANSFORM;

  this->InverseFlag = 0;

  this->PreMultiplyFlag = 1;

  this->NumberOfTransforms = 0;
  this->MaxNumberOfTransforms = 0;
  this->TransformList = NULL;
  this->InverseList = NULL;

  this->UpdateMutex = vtkMutexLock::New();
}

//----------------------------------------------------------------------------
vtkLinearTransformConcatenation::~vtkLinearTransformConcatenation()
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
  if (this->UpdateMutex)
    {
    this->UpdateMutex->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransformConcatenation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLinearTransform::PrintSelf(os,indent);

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
void vtkLinearTransformConcatenation::Concatenate(vtkLinearTransform *trans)
{
  if (trans == this)
    {
    vtkErrorMacro(<< "Concatenate: Can't concatenate with self!");
    return;
    }

  int i;
  vtkLinearTransform **transList = this->TransformList;
  vtkLinearTransform **inverseList = this->InverseList;
  int n = this->NumberOfTransforms;
  this->NumberOfTransforms++;
  
  // check to see if we need to allocate more space
  if (this->NumberOfTransforms > this->MaxNumberOfTransforms)
    {
    int nMax = this->MaxNumberOfTransforms + 20;
    transList = new vtkLinearTransform *[nMax];
    inverseList = new vtkLinearTransform *[nMax];
    for (i = 0; i < n; i++)
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
    for (i = n; i > 0; i--)
      {
      transList[i] = transList[i-1];
      inverseList[i] = inverseList[i-1];
      }
    n = 0;
    }

  if (this->InverseFlag)
    {
    trans = (vtkLinearTransform *)trans->GetInverse();
    }

  transList[n] = trans;
  transList[n]->Register(this);
  inverseList[n] = (vtkLinearTransform *)trans->GetInverse();
  inverseList[n]->Register(this);
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLinearTransformConcatenation::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLinearTransformConcatenation::Identity()
{
  int i;

  if (this->NumberOfTransforms > 0)
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
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
vtkGeneralTransform *vtkLinearTransformConcatenation::MakeTransform()
{
  return vtkLinearTransformConcatenation::New();
}

//----------------------------------------------------------------------------
void vtkLinearTransformConcatenation::DeepCopy(vtkGeneralTransform *transform)
{
  if (this->TransformType != transform->GetTransformType() &&
      this->TransformType != transform->GetInverse()->GetTransformType())
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    }
  if (transform->GetTransformType() & VTK_INVERSE_TRANSFORM)
    {
    transform = ((vtkLinearTransformInverse *)transform)->GetTransform(); 
    }	
  vtkLinearTransformConcatenation *t = 
    (vtkLinearTransformConcatenation *)transform;

  if (t == this)
    {
    return;
    }

  int i;

  this->PreMultiplyFlag = t->PreMultiplyFlag;
  this->InverseFlag = t->InverseFlag;

  if (this->NumberOfTransforms > 0)
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Delete();
      }
    }
  if (this->TransformList)
    {
    delete [] this->TransformList;
    }

  this->MaxNumberOfTransforms = t->MaxNumberOfTransforms;
  this->NumberOfTransforms = t->NumberOfTransforms;

  this->TransformList = 
    new vtkLinearTransform *[this->MaxNumberOfTransforms];

  // copy the transforms by reference
  for (i = 0; i < this->NumberOfTransforms; i++)
    {
    this->TransformList[i] = t->TransformList[i];
    this->TransformList[i]->Register(this);  
    }  
}

//----------------------------------------------------------------------------
void vtkLinearTransformConcatenation::Update()
{
  // lock the update just in case multiple threads try to update
  // at the same time.
  this->UpdateMutex->Lock();

  unsigned long mtime;
  unsigned long matrixMTime = this->Matrix->GetMTime();
  unsigned long maxMTime = 0;

  if (this->InverseFlag)
    {
    for (int i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      mtime = this->TransformList[i]->GetMTime();
      if (mtime > maxMTime)
	{
	maxMTime = mtime;
	}
      }
    if (maxMTime > matrixMTime)
      {
      this->Matrix->Identity();
      for (int i = this->NumberOfTransforms-1; i >= 0; i--)
	{
	vtkLinearTransform *transform = this->TransformList[i];
	transform->Update();
	vtkMatrix4x4::Multiply4x4(transform->GetMatrixPointer(),
				  this->Matrix,this->Matrix);
	}
      }
    }
  else
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      mtime = this->TransformList[i]->GetMTime();
      if (mtime > maxMTime)
	{
	maxMTime = mtime;
	}
      }
    if (maxMTime > matrixMTime)
      {
      this->Matrix->Identity();
      for (int i = 0; i < this->NumberOfTransforms; i++)
	{
	vtkLinearTransform *transform = this->TransformList[i];
	transform->Update();
	vtkMatrix4x4::Multiply4x4(transform->GetMatrixPointer(),
				  this->Matrix,this->Matrix);
	}
      }
    }

  this->UpdateMutex->Unlock();
}

//----------------------------------------------------------------------------
unsigned long vtkLinearTransformConcatenation::GetMTime()
{
  unsigned long result = this->vtkLinearTransform::GetMTime();
  unsigned long mtime;

  if (this->InverseFlag)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      mtime = this->TransformList[i]->GetMTime();
      if (mtime > result)
	{
	result = mtime;
	}
      }
    }
  else
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      mtime = this->InverseList[i]->GetMTime();
      if (mtime > result)
	{
	result = mtime;
	}
      }
    }

  return result;
}

