/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformInverse.cxx
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

