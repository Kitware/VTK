/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObject.cxx
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
#include "vtkDataObject.h"
#include "vtkSource.h"

// Initialize static member that controls global data release after use by filter
static int vtkDataObjectGlobalReleaseDataFlag = 0;

vtkDataObject::vtkDataObject()
{
  this->Source = NULL;
  this->DataReleased = 1;
  this->ReleaseDataFlag = 0;
  this->FieldData = vtkFieldData::New();
}

vtkDataObject::~vtkDataObject()
{
  this->FieldData->Delete();
}

// Determine the modified time of this object
unsigned long int vtkDataObject::GetMTime()
{
  unsigned long result;

  result = vtkObject::GetMTime();
  if ( this->FieldData )
    {
    unsigned long mtime = this->FieldData->GetMTime();
    result = ( mtime > result ? mtime : result);
    }
  return result;
}

void vtkDataObject::Initialize()
{
//
// We don't modify ourselves because the "ReleaseData" methods depend upon
// no modification when initialized.
//
  this->FieldData->Initialize();
};

void vtkDataObject::SetGlobalReleaseDataFlag(int val)
{
  if (val == vtkDataObjectGlobalReleaseDataFlag)
    {
    return;
    }
  vtkDataObjectGlobalReleaseDataFlag = val;
}

int vtkDataObject::GetGlobalReleaseDataFlag()
{
  return vtkDataObjectGlobalReleaseDataFlag;
}

void vtkDataObject::ReleaseData()
{
  this->Initialize();
  this->DataReleased = 1;
}

int vtkDataObject::ShouldIReleaseData()
{
  if ( vtkDataObjectGlobalReleaseDataFlag || this->ReleaseDataFlag )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkDataObject::Update()
{
  if (this->Source)
    {
    this->Source->Update();
    }
}

void vtkDataObject::ForceUpdate()
{
  if (this->Source)
    {
    this->Source->Modified();
    this->Source->Update();
    }
}

void vtkDataObject::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Source )
    {
    os << indent << "Source: " << this->Source << "\n";
    }
  else
    {
    os << indent << "Source: (none)\n";
    }

  os << indent << "Release Data: " << (this->ReleaseDataFlag ? "On\n" : "Off\n");
  os << indent << "Data Released: " << (this->DataReleased ? "True\n" : "False\n");
  os << indent << "Global Release Data: " 
     << (vtkDataObjectGlobalReleaseDataFlag ? "On\n" : "Off\n");

  os << indent << "Field Data:\n";
  this->FieldData->PrintSelf(os,indent.GetNextIndent());
}

void vtkDataObject::SetSource(vtkSource *_arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Source to " << _arg ); 
  if (this->Source != _arg) 
    { 
		vtkSource *tmp = this->Source;
    this->Source = _arg; 
    if (tmp != NULL) { tmp->UnRegister(this); }
    if (this->Source != NULL) { this->Source->Register(this); } 
    this->Modified(); 
    } 
}


//----------------------------------------------------------------------------
void vtkDataObject::UnRegister(vtkObject *o)
{
  // detect the circular loop source <-> data
  // If we have two references and one of them is my data
  // and I am not being unregistered by my data, break the loop.
  if (this->ReferenceCount == 2 && this->Source != NULL &&
      this->Source->GetOutputAsDataObject() == this && 
      o != this->Source &&
      this->Source->GetReferenceCount() == 1)
    {
    this->SetSource(NULL);
    }
  
  this->vtkObject::UnRegister(o);
}
