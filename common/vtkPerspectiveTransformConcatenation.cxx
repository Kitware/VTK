/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransformConcatenation.cxx
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

#include "vtkPerspectiveTransformConcatenation.h"
#include "vtkPerspectiveTransformInverse.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPerspectiveTransformConcatenation *vtkPerspectiveTransformConcatenation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPerspectiveTransformConcatenation");
  if(ret)
    {
    return (vtkPerspectiveTransformConcatenation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPerspectiveTransformConcatenation;
}

//----------------------------------------------------------------------------
vtkPerspectiveTransformConcatenation::vtkPerspectiveTransformConcatenation()
{
  this->InverseFlag = 0;

  this->PreMultiplyFlag = 1;

  this->NumberOfTransforms = 0;
  this->MaxNumberOfTransforms = 0;
  this->TransformList = NULL;
  this->InverseList = NULL;

  this->UpdateRequired = 1;
  this->UpdateMutex = vtkMutexLock::New();
}

//----------------------------------------------------------------------------
vtkPerspectiveTransformConcatenation::~vtkPerspectiveTransformConcatenation()
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
void vtkPerspectiveTransformConcatenation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPerspectiveTransform::PrintSelf(os,indent);

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
void vtkPerspectiveTransformConcatenation::Concatenate(vtkPerspectiveTransform *trans)
{
  if (trans == this)
    {
    vtkErrorMacro(<< "Concatenate: Can't concatenate with self!");
    return;
    }

  vtkPerspectiveTransform **transList = this->TransformList;
  vtkPerspectiveTransform **inverseList = this->InverseList;
  int n = this->NumberOfTransforms;
  this->NumberOfTransforms++;
  
  // check to see if we need to allocate more space
  if (this->NumberOfTransforms > this->MaxNumberOfTransforms)
    {
    int nMax = this->MaxNumberOfTransforms + 20;
    transList = new vtkPerspectiveTransform *[nMax];
    inverseList = new vtkPerspectiveTransform *[nMax];
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
    trans = trans->GetPerspectiveInverse();
    }

  transList[n] = trans;
  transList[n]->Register(this);
  inverseList[n] = trans->GetPerspectiveInverse();
  inverseList[n]->Register(this);

  this->UpdateRequired = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
// concatenate a set of transforms in order.
void vtkPerspectiveTransformConcatenation::Concatenate(
					     vtkPerspectiveTransform *t1,
					     vtkPerspectiveTransform *t2,
					     vtkPerspectiveTransform *t3,
					     vtkPerspectiveTransform *t4)
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

//----------------------------------------------------------------------------
void vtkPerspectiveTransformConcatenation::Inverse()
{
  this->InverseFlag = !this->InverseFlag;

  this->UpdateRequired = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformConcatenation::Identity()
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

  this->UpdateRequired = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkPerspectiveTransformConcatenation::MakeTransform()
{
  return vtkPerspectiveTransformConcatenation::New();
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformConcatenation::DeepCopy(vtkGeneralTransform *transform)
{
  if (strcmp("vtkPerspectiveTransformInverse",transform->GetClassName()) == 0)
    {
    transform = ((vtkPerspectiveTransformInverse *)transform)->GetTransform();
    }
  if (strcmp("vtkPerspectiveTransformConcatenation",transform->GetClassName()) != 0)
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    return;
    }

  vtkPerspectiveTransformConcatenation *t = 
    (vtkPerspectiveTransformConcatenation *)transform;

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
    new vtkPerspectiveTransform *[this->MaxNumberOfTransforms];
  this->InverseList = 
    new vtkPerspectiveTransform *[this->MaxNumberOfTransforms];

  // copy the transforms by reference
  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    this->TransformList[i] = t->TransformList[i];
    this->TransformList[i]->Register(this);  
    this->InverseList[i] = t->InverseList[i];
    this->InverseList[i]->Register(this);  
    }  

  this->UpdateRequired = 1;
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformConcatenation::Update()
{
  // lock the update just in case multiple threads update simultaneously
  this->UpdateMutex->Lock();

  unsigned long maxMTime = 0;

  if (!this->UpdateRequired)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      unsigned long mtime = this->TransformList[i]->GetMTime();
      if (mtime > maxMTime)
	{
	maxMTime = mtime;
	}
      }    
    }
  if (maxMTime > this->Matrix->GetMTime() || this->UpdateRequired)
    {
    this->Matrix->Identity();

    if (this->InverseFlag)
      {
      // concatenate inverse transforms in reverse order
      for (int i = this->NumberOfTransforms-1; i >= 0; i--)
	{
        vtkPerspectiveTransform *transform = this->InverseList[i];
        transform->Update();
	vtkMatrix4x4::Multiply4x4(transform->GetMatrixPointer(),
				  this->Matrix,this->Matrix);
	}
      }
    else
      {
      // concatenate transforms in forward direction
      for (int i = 0; i < this->NumberOfTransforms; i++)
	{
	vtkPerspectiveTransform *transform = this->TransformList[i];
	transform->Update();
	vtkMatrix4x4::Multiply4x4(transform->GetMatrixPointer(),
				  this->Matrix,this->Matrix);
	}
      }
    this->UpdateRequired = 0;
    }
  
  this->UpdateMutex->Unlock();
}

//----------------------------------------------------------------------------
unsigned long vtkPerspectiveTransformConcatenation::GetMTime()
{
  unsigned long result = this->vtkPerspectiveTransform::GetMTime();

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

