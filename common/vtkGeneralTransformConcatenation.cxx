/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformConcatenation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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
      if (this->InverseTransformList[i]->GetAutoUpdate())
	{
	this->InverseTransformList[i]->AutoUpdateOff();
	this->InverseTransformList[i]->TransformPoint(output,output);
	this->InverseTransformList[i]->AutoUpdateOn();
	}
      else
	{
	this->InverseTransformList[i]->TransformPoint(output,output);
	}
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      if (this->TransformList[i]->GetAutoUpdate())
	{
	this->TransformList[i]->AutoUpdateOff();
	this->TransformList[i]->TransformPoint(output,output);
	this->TransformList[i]->AutoUpdateOn();
	}
      else
	{
	this->TransformList[i]->TransformPoint(output,output);
	}
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

  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      if (this->InverseTransformList[i]->GetAutoUpdate())
	{
	this->InverseTransformList[i]->AutoUpdateOff();
	this->InverseTransformList[i]->TransformPoint(output,output);
	this->InverseTransformList[i]->AutoUpdateOn();
	}
      else
	{
	this->InverseTransformList[i]->TransformPoint(output,output);
	}
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      if (this->TransformList[i]->GetAutoUpdate())
	{
	this->TransformList[i]->AutoUpdateOff();
	this->TransformList[i]->TransformPoint(output,output);
	this->TransformList[i]->AutoUpdateOn();
	}
      else
	{
	this->TransformList[i]->TransformPoint(output,output);
	}
      }
    }
}    

//----------------------------------------------------------------------------
void vtkGeneralTransformConcatenation::TransformPoints(vtkPoints *in, 
						       vtkPoints *out)
{
  int i,j;
  int n = in->GetNumberOfPoints();
  int m = out->GetNumberOfPoints();

  if (this->NumberOfTransforms == 0)
    {
    for (j = 0; j < n; j++)
      {
      out->InsertNextPoint(in->GetPoint(j));
      }
    return;
    }      

  vtkPoints *points;
  vtkPoints *tmpPoints = NULL;

  if (this->NumberOfTransforms > 1)
    {
    tmpPoints = vtkPoints::New();
    tmpPoints->SetNumberOfPoints(n);
    }

  points = in;
  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseTransformList[i]->TransformPoints(points,out);
      if (i == 0)
	{
	break;
	}
      points = tmpPoints;
      for (j = 0; j < n; j++)
	{
	points->SetPoint(j,out->GetPoint(j+m));
	}
      out->SetNumberOfPoints(m);
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->TransformPoints(points,out);
      if (i == this->NumberOfTransforms+1)
	{
	break;
	}
      points = tmpPoints;
      for (j = 0; j < n; j++)
	{
	points->SetPoint(j,out->GetPoint(j+m));
	}
      out->SetNumberOfPoints(m);
      }
    }

  if (tmpPoints)
    {
    tmpPoints->Delete();
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

  if (this->NumberOfTransforms == 0)
    {
    for (j = 0; j < n; j++)
      {
      outNms->InsertNextNormal(inNms->GetNormal(j));
      }
    return;
    }      

  vtkNormals *normals;
  vtkNormals *tmpNormals = NULL;

  if (this->NumberOfTransforms > 1)
    {
    tmpNormals = vtkNormals::New();
    tmpNormals->SetNumberOfNormals(n);
    }

  normals = inNms;
  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseTransformList[i]->TransformNormals(inPts,outPts,
						      normals,outNms);
      if (i == 0)
	{
	break;
	}
      normals = tmpNormals;
      for (j = 0; j < n; j++)
	{
	normals->SetNormal(j,outNms->GetNormal(j+m));
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
      if (i == this->NumberOfTransforms+1)
	{
	break;
	}
      normals = tmpNormals;
      for (j = 0; j < n; j++)
	{
	normals->SetNormal(j,outNms->GetNormal(j+m));
	}
      outNms->SetNumberOfNormals(m);
      }
    }

  if (tmpNormals)
    {
    tmpNormals->Delete();
    }
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
  vtkVectors *tmpVectors = NULL;

  if (this->NumberOfTransforms > 1)
    {
    tmpVectors = vtkVectors::New();
    tmpVectors->SetNumberOfVectors(n);
    }

  vectors = inVrs;
  if (this->InverseFlag)
    {
    for (i = this->NumberOfTransforms-1; i >= 0; i--)
      {
      this->InverseTransformList[i]->TransformVectors(inPts,outPts,
						      vectors,outVrs);
      if (i == 0)
	{
	break;
	}
      vectors = tmpVectors;
      for (j = 0; j < n; j++)
	{
	vectors->SetVector(j,outVrs->GetVector(j+m));
	}
      outVrs->SetNumberOfVectors(m);
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfTransforms; i++)
      {
      this->TransformList[i]->TransformVectors(inPts,outPts,
					       vectors,outVrs);
      if (i == this->NumberOfTransforms+1)
	{
	break;
	}
      vectors = tmpVectors;
      for (j = 0; j < n; j++)
	{
	vectors->SetVector(j,outVrs->GetVector(j+m));
	}
      outVrs->SetNumberOfVectors(m);
      }
    }

  if (tmpVectors)
    {
    tmpVectors->Delete();
    }
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

