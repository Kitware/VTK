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
  this->TransformType = VTK_CONCATENATION_TRANSFORM;

  this->PreMultiplyFlag = 1;
  this->InverseFlag = 0;

  this->NumberOfTransforms = 0;
  this->MaxNumberOfTransforms = 0;
  this->TransformList = NULL;
  this->InverseTransformList = NULL;
}

//----------------------------------------------------------------------------
vtkGeneralTransformConcatenation::~vtkGeneralTransformConcatenation()
{
  int i;

  if (this->NumberOfTransforms > 0)
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Delete();
      this->InverseTransformList[i]->Delete();
      }
    }
  if (this->TransformList)
    {
    delete [] this->TransformList;
    }
  if (this->InverseTransformList)
    {
    delete [] this->InverseTransformList;
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
  int i;
  for (i = 0; i < this->NumberOfTransforms; i++)
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

  int i;
  vtkGeneralTransform **transList = this->TransformList;
  vtkGeneralTransform **invTransList = this->InverseTransformList;
  int n = this->NumberOfTransforms;
  this->NumberOfTransforms++;
  
  trans->Register(this);

  // check to see if we need to allocate more space
  if (this->NumberOfTransforms > this->MaxNumberOfTransforms)
    {
    int nMax = this->MaxNumberOfTransforms + 20;
    transList = new vtkGeneralTransform *[nMax];
    invTransList = new vtkGeneralTransform *[nMax];
    for (i = 0; i < n; i++)
      {
      transList[i] = this->TransformList[i];
      invTransList[i] = this->InverseTransformList[i];
      }
    if (this->TransformList)
      {
      delete [] this->TransformList;
      }
    if (this->InverseTransformList)
      {
      delete [] this->InverseTransformList;
      }
    this->TransformList = transList;
    this->InverseTransformList = invTransList;
    this->MaxNumberOfTransforms = nMax;
    }

  vtkGeneralTransform *invTrans = trans->GetInverse();
  invTrans->Register(this);

  // add the transform either the beginning or end of the list,
  // according to flags
  if (this->PreMultiplyFlag ^ this->InverseFlag)
    {
    for (i = n; i > 0; i--)
      {
      transList[i] = transList[i-1];
      invTransList[i] = invTransList[i-1];
      }
    n = 0;
    }

  if (this->InverseFlag)
    {
    transList[n] = invTrans;
    invTransList[n] = trans;
    }
  else
    {
    transList[n] = trans;
    invTransList[n] = invTrans;
    }  
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::TransformPoint(const float input[3], 
						      float output[3])
{
  int i;
  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];

  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseTransformList[i]->TransformPoint(output,output);
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->TransformPoint(output,output);
      }
    }
}    

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::TransformPoint(const double input[3], 
						      double output[3])
{
  int i;
  output[0] = input[0];
  output[1] = input[1];
  output[2] = input[2];

  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseTransformList[i]->TransformPoint(output,output);
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->TransformPoint(output,output);
      }
    }
}    

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::TransformPoints(vtkPoints *in, 
						       vtkPoints *out)
{
  int i,j;
  float point[3];
  int n = in->GetNumberOfPoints();
  int m = out->GetNumberOfPoints();

  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->NumberOfTransforms == 0)
    {
    for (j = 0; j < n; j++)
      {
      out->InsertNextPoint(in->GetPoint(j));
      }
    return;
    }      

  if (this->InverseFlag)
    {
    for (j = 0; j < n; j++)
      {
      in->GetPoint(j,point);
      for (i = this->NumberOfTransforms-1; i >= 0; i--)
	{
	this->InverseTransformList[i]->TransformPoint(point,point);
	}
      out->InsertNextPoint(point);
      }
    }
  else
    {
    for (j = 0; j < n; j++)
      {
      in->GetPoint(j,point);
      for (i = 0; i < this->NumberOfTransforms; i++)
	{
	this->TransformList[i]->TransformPoint(point,point);
	}
      out->InsertNextPoint(point);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::TransformNormals(vtkPoints *inPts, 
							vtkPoints *outPts,
							vtkNormals *inNms,
							vtkNormals *outNms)
{
  int i,j;
  int n = inNms->GetNumberOfNormals();
  int m = outNms->GetNumberOfNormals();

  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->NumberOfTransforms == 0)
    {
    for (j = 0; j < n; j++)
      {
      outNms->InsertNextNormal(inNms->GetNormal(j));
      }
    return;
    }      

  vtkNormals *normals;
  vtkNormals *tmpNormals = vtkNormals::New();

  normals = inNms;
  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; ; i--)
      {
      this->InverseTransformList[i]->TransformNormals(inPts,outPts,
						      normals,outNms);
      if (i == 0)
	{
	break;
	}
      normals = tmpNormals;
      normals->SetNumberOfNormals(0);
      for (j = 0; j < n; j++)
	{
	normals->InsertNextNormal(outNms->GetNormal(j+m));
	}
      outNms->SetNumberOfNormals(m);
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->TransformNormals(inPts,outPts,
					       normals,outNms);
      if (i == this->NumberOfTransforms)
	{
	break;
	}
      normals = tmpNormals;
      normals->SetNumberOfNormals(0);
      for (j = 0; j < n; j++)
	{
	normals->InsertNextNormal(outNms->GetNormal(j+m));
	}
      outNms->SetNumberOfNormals(m);
      }
    }

  tmpNormals->Delete();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::TransformVectors(vtkPoints *inPts, 
							vtkPoints *outPts,
							vtkVectors *inVrs,
							vtkVectors *outVrs)
{
  int i,j;
  int n = inVrs->GetNumberOfVectors();
  int m = outVrs->GetNumberOfVectors();

  if (this->NumberOfTransforms == 0)
    {
    for (j = 0; j < n; j++)
      {
      outVrs->InsertNextVector(inVrs->GetVector(j));
      }
    return;
    }      

  vtkVectors *vectors;
  vtkVectors *tmpVectors = vtkVectors::New();

  vectors = inVrs;
  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; ; i--)
      {
      this->InverseTransformList[i]->TransformVectors(inPts,outPts,
						      vectors,outVrs);
      if (i == 0)
	{
	break;
	}
      vectors = tmpVectors;
      vectors->SetNumberOfVectors(0);
      for (j = 0; j < n; j++)
	{
	vectors->InsertNextVector(outVrs->GetVector(j+m));
	}
      outVrs->SetNumberOfVectors(m);
      }
    }
  else
    {
    for (i = 0; ; i++)
      {
      this->TransformList[i]->TransformVectors(inPts,outPts,
					       vectors,outVrs);
      if (i == this->NumberOfTransforms)
	{
	break;
	}
      vectors = tmpVectors;
      vectors->SetNumberOfVectors(0);      
      for (j = 0; j < n; j++)
	{
	vectors->InsertNextVector(outVrs->GetVector(j+m));
	}
      outVrs->SetNumberOfVectors(m);
      }
    }

  tmpVectors->Delete();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Identity()
{
  int i;

  if (this->NumberOfTransforms > 0)
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->Delete();
      this->InverseTransformList[i]->Delete();
      }
    }
  this->NumberOfTransforms = 0;
  this->InverseFlag = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
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
  if (transform->GetTransformType() & VTK_INVERSE_TRANSFORM)
    {
    transform = 
      ((vtkGeneralTransformInverse *)transform)->GetInverseTransform(); 
    }	
  if (this->TransformType != transform->GetTransformType())
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    }
  vtkGeneralTransformConcatenation *t = 
    (vtkGeneralTransformConcatenation *)transform;

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
      this->InverseTransformList[i]->Delete();
      }
    }
  if (this->TransformList)
    {
    delete [] this->TransformList;
    }
  if (this->InverseTransformList)
    {
    delete [] this->InverseTransformList;
    }

  this->MaxNumberOfTransforms = t->MaxNumberOfTransforms;
  this->NumberOfTransforms = t->NumberOfTransforms;

  this->TransformList = 
    new vtkGeneralTransform *[this->MaxNumberOfTransforms];
  this->InverseTransformList = 
    new vtkGeneralTransform *[this->MaxNumberOfTransforms];

  // copy the transforms by reference
  for (i = 0; i < this->NumberOfTransforms; i++)
    {
    this->TransformList[i] = t->TransformList[i];
    this->TransformList[i]->Register(this);  
    this->InverseTransformList[i] = t->InverseTransformList[i];
    this->InverseTransformList[i]->Register(this);
    }  
}

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::Update()
{
  int i;
  vtkGeneralTransform **transforms;
  if (this->InverseFlag)
    {
    transforms = this->InverseTransformList;
    }
  else
    {
    transforms = this->TransformList;
    }
  
  for (i = 0; i < this->NumberOfTransforms; i++)
    {
    transforms[i]->Update();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkGeneralTransformConcatenation::GetMTime()
{
  unsigned long result = this->vtkGeneralTransform::GetMTime();
  unsigned long mtime;

  int i;
  vtkGeneralTransform **transforms;
  if (this->InverseFlag)
    {
    transforms = this->InverseTransformList;
    }
  else
    {
    transforms = this->TransformList;
    }
  
  for (i = 0; i < this->NumberOfTransforms; i++)
    {
    mtime = transforms[i]->GetMTime();
    if (mtime > result)
      {
      result = mtime;
      }
    }

  return result;
}

