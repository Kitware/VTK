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
  this->FieldData.ReferenceCountingOff();//because it's always deleted with data object
}

vtkDataObject::~vtkDataObject()
{
}

void vtkDataObject::Initialize()
{
//
// We don't modify ourselves because the "ReleaseData" methods depend upon
// no modification when initialized.
//
  this->FieldData.Initialize();
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

void vtkDataObject::SetFieldData(vtkFieldData *fd)
{
  this->FieldData.ShallowCopy(*fd);
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
  this->FieldData.PrintSelf(os,indent.GetNextIndent());
}

