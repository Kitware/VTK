/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.cxx
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

#include "vtkGeneralTransform.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkGeneralTransform::vtkGeneralTransform()
{
  this->MyInverse = NULL;
  this->DependsOnInverse = 0;
  this->Concatenation = NULL;
  this->InUnRegister = 0;
}

//----------------------------------------------------------------------------
vtkGeneralTransform::~vtkGeneralTransform()
{
  if (this->MyInverse) 
    { 
    this->MyInverse->Delete(); 
    } 
  if (this->Concatenation)
    { 
    this->Concatenation->Delete(); 
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent << "Inverse: (" << this->MyInverse << ")\n";
}

//----------------------------------------------------------------------------
// These two functions are definitely not thread safe, and should
// really only be called from python or tcl.
float *vtkGeneralTransform::TransformFloatPoint(float x, 
						float y, 
						float z)
{
  this->InternalFloatPoint[0] = x;
  this->InternalFloatPoint[1] = y;
  this->InternalFloatPoint[2] = z;
  this->TransformPoint(this->InternalFloatPoint,this->InternalFloatPoint);
  return this->InternalFloatPoint;
}

//----------------------------------------------------------------------------
double *vtkGeneralTransform::TransformDoublePoint(double x,
						  double y,
						  double z)
{
  this->InternalDoublePoint[0] = x;
  this->InternalDoublePoint[1] = y;
  this->InternalDoublePoint[2] = z;
  this->TransformPoint(this->InternalDoublePoint,this->InternalDoublePoint);
  return this->InternalDoublePoint;
}

//----------------------------------------------------------------------------
// Transform a series of points.
void vtkGeneralTransform::TransformPoints(vtkPoints *in, 
					  vtkPoints *out)
{
  this->Update();

  double point[3];
  int i;
  int n = in->GetNumberOfPoints();

  for (i = 0; i < n; i++)
    {
    in->GetPoint(i,point);
    this->InternalTransformPoint(point,point);
    out->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 

void vtkGeneralTransform::TransformPointsNormalsVectors(vtkPoints *inPts,
							vtkPoints *outPts,
							vtkNormals *inNms, 
							vtkNormals *outNms,
							vtkVectors *inVrs,
							vtkVectors *outVrs)
{
  this->Update();

  double matrix[3][3];
  double coord[3];

  int i;
  int n = inPts->GetNumberOfPoints();

  for (i = 0; i < n; i++)
    {
    inPts->GetPoint(i,coord);
    this->InternalTransformDerivative(coord,coord,matrix);
    outPts->InsertNextPoint(coord);
    
    if (inVrs)
      {
      inVrs->GetVector(i,coord);
      vtkMath::Multiply3x3(matrix,coord,coord);
      outVrs->InsertNextVector(coord);
      }
    
    if (inNms)
      {
      inNms->GetNormal(i,coord);
      vtkMath::Transpose3x3(matrix,matrix);
      vtkMath::LinearSolve3x3(matrix,coord,coord);
      vtkMath::Normalize(coord);
      outNms->InsertNextNormal(coord);
      }
    }
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransform::GetInverse()
{
  if (this->MyInverse == NULL)
    {
    // we create a circular reference here, it is dealt with in UnRegister
    this->MyInverse = this->MakeTransform();
    this->MyInverse->SetInverse(this);
    }
  return this->MyInverse;
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::SetInverse(vtkGeneralTransform *transform)
{
  if (this->MyInverse == transform)
    {
    return;
    }

  // check type first
  if (!transform->IsA(this->GetClassName()))
    {
    vtkErrorMacro("SetInverse: requires a " << this->GetClassName() << ", a "
		  << transform->GetClassName() << " is not compatible.");
    return;
    }
  
  if (this->MyInverse)
    {
    this->MyInverse->Delete();
    }

  transform->Register(this);
  this->MyInverse = transform;

  // we are now a special 'inverse transform'
  this->DependsOnInverse = (transform != 0);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::DeepCopy(vtkGeneralTransform *transform)
{
  // check whether we're trying to copy a transform to itself
  if (transform == this)
    {
    return;
    }

  // check to see if the transform is the same type as this one
  if (transform->IsA(this->GetClassName()))
    {
    // same type, so we do the real DeepCopy. 
    this->InternalDeepCopy(transform);
    }
  // otherwise, we're out of luck
  else
    {
    vtkErrorMacro("DeepCopy: can't copy a " << transform->GetClassName()
		  << " into a " << this->GetClassName() << ".");
    }

  return;
}    

//----------------------------------------------------------------------------
void vtkGeneralTransform::Update()
{
  // locking is require to ensure that the class is thread-safe
  this->UpdateMutex.Lock();

  // check to see if we are a special 'inverse' transform
  if (this->DependsOnInverse)
    {
    // just check to see when Modified() was last called
    if (this->MyInverse->vtkObject::GetMTime() >= this->UpdateTime.GetMTime())
      {
      this->DeepCopy(this->MyInverse);
      this->Inverse();
      this->InternalUpdate();
      }
    }

  // otherwise just check our MTime against our last update
  else if (this->GetMTime() >= this->UpdateTime.GetMTime())
    {
    this->InternalUpdate();
    }

  this->UpdateTime.Modified();
  this->UpdateMutex.Unlock();
}

//----------------------------------------------------------------------------
// Need to check inverse's MTime if we are an inverse transform
unsigned long vtkGeneralTransform::GetMTime()
{
  unsigned long mtime = this->vtkObject::GetMTime();
  if (this->DependsOnInverse)
    {
    unsigned long inverseMTime = this->MyInverse->GetMTime();
    if (inverseMTime > mtime)
      {
      return inverseMTime;
      }
    }
  return mtime;
}

//----------------------------------------------------------------------------
// We need to handle the circular reference between a transform and its
// inverse.
void vtkGeneralTransform::UnRegister(vtkObject *o)
{
  if (this->InUnRegister)
    { // we don't want to go into infinite recursion...
    this->ReferenceCount--;
    return;
    }

  // check to see if the only reason our reference count is not 1
  // is the circular reference from MyInverse
  if (this->MyInverse && this->ReferenceCount == 2 &&
      this->MyInverse->ReferenceCount == 1)
    { // break the cycle
    this->InUnRegister = 1;
    this->MyInverse->Delete();
    this->MyInverse = NULL;
    this->InUnRegister = 0;
    }

  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// All of the following methods are for vtkSimpleTransformConcatenation
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkSimpleTransformConcatenation::vtkSimpleTransformConcatenation(
                                           vtkGeneralTransform *transform)
{
  this->Transform = transform;

  this->InverseFlag = 0;

  this->PreMultiplyFlag = 1;

  this->NumberOfTransforms = 0;
  this->MaxNumberOfTransforms = 0;

  // The transform list is the list of the transforms to be concatenated.
  this->TransformList = NULL;

  // The inverse list holds the inverses of these same transforms.
  this->InverseList = NULL;
}

//----------------------------------------------------------------------------
vtkSimpleTransformConcatenation::~vtkSimpleTransformConcatenation()
{
  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      if (this->TransformList[i])
	{
	this->TransformList[i]->Delete();
	}
      if (this->InverseList[i])
	{
        this->InverseList[i]->Delete();
	}
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
void vtkSimpleTransformConcatenation::Concatenate(vtkGeneralTransform *trans)
{
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

  trans->Register(this->Transform);
  
  if (this->InverseFlag)
    {
    transList[n] = NULL;
    inverseList[n] = trans;
    }
  else
    {
    transList[n] = trans;
    inverseList[n] = NULL;
    }

  this->Transform->Modified();
}

//----------------------------------------------------------------------------
// concatenate a set of transforms in order.
void vtkSimpleTransformConcatenation::Concatenate(vtkGeneralTransform *t1,
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

//----------------------------------------------------------------------------
void vtkSimpleTransformConcatenation::Inverse()
{
  this->InverseFlag = !this->InverseFlag;

  this->Transform->Modified();
}

//----------------------------------------------------------------------------
void vtkSimpleTransformConcatenation::Identity()
{
  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      if (this->TransformList[i])
	{
	this->TransformList[i]->Delete();
	}
      if (this->InverseList[i])
	{
        this->InverseList[i]->Delete();
	}
      }
    }
  this->NumberOfTransforms = 0;
  this->InverseFlag = 0;

  this->Transform->Modified();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkSimpleTransformConcatenation::GetTransform(int i)
{
  vtkGeneralTransform *transform, *inverse;

  // we walk through the list in reverse order if InverseFlag is set
  if (this->InverseFlag)
    {
    transform = this->InverseList[this->NumberOfTransforms-i-1];
    inverse = this->TransformList[this->NumberOfTransforms-i-1];
    }
  else
    {
    transform = this->TransformList[i];
    inverse = this->InverseList[i];
    }

  // if the transform is null, get it from the inverse list
  if (transform == NULL)
    {
    transform = inverse->GetInverse();
    }
  
  return transform;
}

//----------------------------------------------------------------------------
unsigned long vtkSimpleTransformConcatenation::GetMaxMTime()
{
  unsigned long result = 0;
  unsigned long mtime;

  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    mtime = this->TransformList[i]->GetMTime();

    if (mtime > result)
      {
      result = mtime;
      }
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkSimpleTransformConcatenation::DeepCopy(
                              vtkSimpleTransformConcatenation *concatenation)
{
  this->Identity();

  if (this->TransformList)
    {
    delete [] this->TransformList;
    }
  if (this->InverseList)
    {
    delete [] this->InverseList;
    }

  this->PreMultiplyFlag = concatenation->PreMultiplyFlag;
  this->InverseFlag = concatenation->InverseFlag;

  this->MaxNumberOfTransforms = concatenation->MaxNumberOfTransforms;
  this->NumberOfTransforms = concatenation->NumberOfTransforms;

  this->TransformList = new vtkGeneralTransform *[this->MaxNumberOfTransforms];
  this->InverseList =   new vtkGeneralTransform *[this->MaxNumberOfTransforms];

  // copy the transforms by reference
  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    if ((this->TransformList[i] = concatenation->TransformList[i]))
      {
      this->TransformList[i]->Register(this->Transform);
      }
    if ((this->InverseList[i] = concatenation->InverseList[i]))
      {
      this->InverseList[i]->Register(this->Transform);  
      }
    } 
}

//----------------------------------------------------------------------------
void vtkSimpleTransformConcatenation::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << (this->PreMultiplyFlag ? "PreMultiply\n" : "PostMultiply\n");
  os << indent << "NumberOfTransforms: " << this->NumberOfTransforms << "\n"; 
  os << indent << "Concatenation:\n";

  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    this->GetTransform(i)->PrintSelf(os,indent.GetNextIndent());
    }
}


