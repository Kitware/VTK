/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkProp.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkProp* vtkProp::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProp");
  if(ret)
    {
    return (vtkProp*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProp;
}




// Creates an Prop with the following defaults: visibility on.
vtkProp::vtkProp()
{
  this->Visibility = 1;  // ON

  this->Pickable   = 1;
  this->PickMethod = NULL;
  this->PickMethodArgDelete = NULL;
  this->PickMethodArg = NULL;
  this->Dragable   = 1;
  
  this->AllocatedRenderTime = 10.0;
  this->EstimatedRenderTime = 0.0;
  this->RenderTimeMultiplier = 1.0;
}

vtkProp::~vtkProp()
{
  if ((this->PickMethodArg)&&(this->PickMethodArgDelete))
    {
    (*this->PickMethodArgDelete)(this->PickMethodArg);
    }
}

// This method is invoked when an instance of vtkProp (or subclass, 
// e.g., vtkActor) is picked by vtkPicker.
void vtkProp::SetPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->PickMethod || arg != this->PickMethodArg )
    {
    // delete the current arg if there is one and a delete method
    if ((this->PickMethodArg)&&(this->PickMethodArgDelete))
      {
      (*this->PickMethodArgDelete)(this->PickMethodArg);
      }
    this->PickMethod = f;
    this->PickMethodArg = arg;
    this->Modified();
    }
}

// Set a method to delete user arguments for PickMethod.
void vtkProp::SetPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->PickMethodArgDelete)
    {
    this->PickMethodArgDelete = f;
    this->Modified();
    }
}

// This method is invoked if the prop is picked.
void vtkProp::Pick()
  {
  if (this->PickMethod)
    {
    (*this->PickMethod)(this->PickMethodArg);
    }
  }

// Shallow copy of vtkProp.
void vtkProp::ShallowCopy(vtkProp *Prop)
{
  this->Visibility = Prop->GetVisibility();
  this->Pickable   = Prop->GetPickable();
  this->Dragable   = Prop->GetDragable();

  this->SetPickMethod(Prop->PickMethod, Prop->PickMethodArg);
  this->SetPickMethodArgDelete(Prop->PickMethodArgDelete);
}

void vtkProp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");

  if ( this->PickMethod )
    {
    os << indent << "Pick Method defined\n";
    }
  else
    {
    os << indent <<"No Pick Method\n";
    }

  os << indent << "AllocatedRenderTime: " 
     << this->AllocatedRenderTime << endl;
  os << indent << "EstimatedRenderTime: " 
     << this->EstimatedRenderTime << endl;
  os << indent << "RenderTimeMultiplier: " 
     << this->RenderTimeMultiplier << endl;
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");
}




