/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransformInverse.cxx
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

#include "vtkPerspectiveTransformInverse.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPerspectiveTransformInverse* vtkPerspectiveTransformInverse::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPerspectiveTransformInverse");
  if(ret)
    {
    return (vtkPerspectiveTransformInverse*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPerspectiveTransformInverse;
}

//----------------------------------------------------------------------------
vtkPerspectiveTransformInverse::vtkPerspectiveTransformInverse()
{
  this->Transform = NULL;
  this->UpdateRequired = 0;
  this->UpdateMutex = vtkMutexLock::New();
}

//----------------------------------------------------------------------------
vtkPerspectiveTransformInverse::~vtkPerspectiveTransformInverse()
{
  if (this->Transform)
    {
    this->Transform->Delete();
    }
  if (this->UpdateMutex)
    {
    this->UpdateMutex->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformInverse::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkPerspectiveTransform::PrintSelf(os,indent);

  os << indent << "Transform: " << this->Transform << "\n";
  if (this->Transform)
    {
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformInverse::SetInverse(vtkPerspectiveTransform *trans)
{
  if (this == trans)
    {
    vtkErrorMacro(<<"SetInverse: A transform cannot be its own inverse!");
    return;
    }
  if (this->MyInverse == trans)
    {
    return;
    }
  if (this->MyInverse)
    {
    this->MyInverse->Delete();
    this->Transform->Delete();
    }
  this->MyInverse = trans;
  trans->Register(this);
  this->Transform = (vtkPerspectiveTransform *)trans->MakeTransform();

  this->UpdateRequired = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkPerspectiveTransformInverse::GetInverse()
{
  return (vtkPerspectiveTransform *)this->MyInverse;
}

//----------------------------------------------------------------------------
vtkPerspectiveTransform *vtkPerspectiveTransformInverse::GetTransform()
{
  return this->Transform;
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformInverse::Identity()
{
  if (this->MyInverse == NULL)
    {
    vtkErrorMacro(<< "Identity: Inverse has not been set");
    return;
    }
  this->MyInverse->Identity();
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformInverse::Inverse()
{
  if (this->MyInverse == NULL)
    {
    vtkErrorMacro(<< "Inverse: Inverse has not been set");
    return;
    }
  this->MyInverse->Inverse();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkPerspectiveTransformInverse::MakeTransform()
{
  if (this->MyInverse == NULL)
    {
    vtkErrorMacro(<< "MakeTransform: Inverse has not been set");
    return NULL;
    }
  return this->MyInverse->MakeTransform();
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformInverse::DeepCopy(vtkGeneralTransform *transform)
{
  if (this->MyInverse == NULL)
    {
    vtkErrorMacro(<< "DeepCopy: Inverse has not been set");
    return;
    }
  this->MyInverse->DeepCopy(transform);
  this->MyInverse->Inverse();
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransformInverse::Update()
{
  // lock the update just in case multiple threads update simultaneously
  this->UpdateMutex->Lock();

  if (this->MyInverse->GetMTime() > this->Matrix->GetMTime() 
      || this->UpdateRequired)
    {
    this->Transform->DeepCopy(this->MyInverse);
    this->Transform->Inverse();
    this->Transform->GetMatrix(this->Matrix);
    this->UpdateRequired = 0;
    }

  this->UpdateMutex->Unlock();
}

//----------------------------------------------------------------------------
unsigned long vtkPerspectiveTransformInverse::GetMTime()
{
  unsigned long result = this->vtkPerspectiveTransform::GetMTime();

  if (this->MyInverse)
    {
    unsigned long mtime = this->MyInverse->GetMTime();
    if (mtime > result)
      {
      result = mtime;
      }
    }
  return result;
}



