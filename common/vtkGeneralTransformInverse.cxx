/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformInverse.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkGeneralTransformInverse.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkGeneralTransformInverse* vtkGeneralTransformInverse::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGeneralTransformInverse");
  if(ret)
    {
    return (vtkGeneralTransformInverse*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGeneralTransformInverse;
}

//----------------------------------------------------------------------------
vtkGeneralTransformInverse::vtkGeneralTransformInverse()
{
  this->TransformType = VTK_INVERSE_TRANSFORM;

  this->OriginalTransform = NULL;
  this->InverseTransform = NULL;
  this->UpdateRequired = 0;
}

//----------------------------------------------------------------------------
vtkGeneralTransformInverse::~vtkGeneralTransformInverse()
{
  if (this->OriginalTransform)
    {
    this->OriginalTransform->Delete();
    }
  if (this->InverseTransform)
    {
    this->InverseTransform->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGeneralTransform::PrintSelf(os,indent);

  os << indent << "OriginalTransform: " << this->OriginalTransform << "\n";
  os << indent << "InverseTransform: " << this->InverseTransform << "\n";
  if (this->InverseTransform)
    {
    this->InverseTransform->PrintSelf(os,indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::SetOriginalTransform(vtkGeneralTransform 
						      *trans)
{
  if (this == trans)
    {
    vtkErrorMacro(<<"SetOriginalTransform: A transform cannot be its own inverse!");
    return;
    }
  if (this->OriginalTransform == trans)
    {
    return;
    }
  if (this->OriginalTransform)
    {
    this->OriginalTransform->Delete();
    this->InverseTransform->Delete();
    }
  this->OriginalTransform = trans;
  trans->Register(this);
  this->InverseTransform = trans->MakeTransform();
  this->UpdateRequired = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::TransformPoint(const float input[3], 
						float output[3])
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->OriginalTransform == NULL)
    {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    return;
    }

  this->InverseTransform->TransformPoint(input,output);
}    

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::TransformPoint(const double input[3], 
						double output[3])
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->OriginalTransform == NULL)
    {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    return;
    }

  this->InverseTransform->TransformPoint(input,output);
}    

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::TransformPoints(vtkPoints *in, vtkPoints *out)
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  int i;
  int n = in->GetNumberOfPoints();

  if (this->OriginalTransform == NULL)
    {
    for (i = 0; i < n; i++)
      {
      out->InsertNextPoint(in->GetPoint(i));
      }
    return;
    }      

  this->InverseTransform->TransformPoints(in,out);
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::TransformNormals(vtkPoints *inPts, 
						  vtkPoints *outPts,
						  vtkNormals *in, 
						  vtkNormals *out)
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  int i;
  int n = in->GetNumberOfNormals();

  if (this->OriginalTransform == NULL)
    {
    for (i = 0; i < n; i++)
      {
      out->InsertNextNormal(in->GetNormal(i));
      }
    return;
    }      

  this->InverseTransform->TransformNormals(inPts,outPts,in,out);
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::TransformVectors(vtkPoints *inPts, 
						  vtkPoints *outPts,
						  vtkVectors *in, 
						  vtkVectors *out)
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  int i;
  int n = in->GetNumberOfVectors();

  if (this->OriginalTransform == NULL)
    {
    for (i = 0; i < n; i++)
      {
      out->InsertNextVector(in->GetVector(i));
      }
    return;
    }      

  this->InverseTransform->TransformVectors(inPts,outPts,in,out);
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransformInverse::GetInverse()
{
  if (this->OriginalTransform == NULL)
    {
    vtkErrorMacro(<< "GetInverse: OriginalTransform has not been set");
    return NULL;
    }
  return this->OriginalTransform;
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::Identity()
{
  this->OriginalTransform->Identity();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::Inverse()
{
  this->OriginalTransform->Inverse();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkGeneralTransformInverse::MakeTransform()
{
  return this->OriginalTransform->MakeTransform();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::DeepCopy(vtkGeneralTransform *transform)
{
  this->OriginalTransform->DeepCopy(transform);
  this->OriginalTransform->Inverse();
}

//----------------------------------------------------------------------------
void vtkGeneralTransformInverse::Update()
{
  if (this->OriginalTransform == NULL)
    {
    return;
    }

  this->OriginalTransform->Update();

  if (this->OriginalTransform->GetMTime() > 
      this->InverseTransform->GetMTime() || this->UpdateRequired)
    {
    this->InverseTransform->DeepCopy(this->OriginalTransform);
    this->InverseTransform->Inverse();
    this->UpdateRequired = 0;
    }

  this->InverseTransform->Update();
}

//----------------------------------------------------------------------------
unsigned long vtkGeneralTransformInverse::GetMTime()
{
  unsigned long result = this->vtkGeneralTransform::GetMTime();
  unsigned long mtime;

  if (this->OriginalTransform)
    {
    mtime = this->OriginalTransform->GetMTime();
    if (mtime > result)
      {
      result = mtime;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
// need to handle the circular references
void vtkGeneralTransformInverse::UnRegister(vtkObject *o)
{
  if (this->InUnRegister)
    {
    this->ReferenceCount--;
    return;
    }

  // 'this' is referenced by this->OriginalTransform
  if (this->ReferenceCount == 2 && this->OriginalTransform && 
      this->OriginalTransform->GetReferenceCount() == 1 &&
      this->OriginalTransform->GetInverse() == this)
    { // break the cycle
    this->InUnRegister = 1;
    this->OriginalTransform->Delete();
    this->OriginalTransform = NULL;
    this->InUnRegister = 0;
    }

  this->vtkGeneralTransform::UnRegister(o);
}

