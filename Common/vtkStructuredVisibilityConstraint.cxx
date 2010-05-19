/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredVisibilityConstraint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredVisibilityConstraint.h"

#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkStructuredVisibilityConstraint);

vtkCxxSetObjectMacro(vtkStructuredVisibilityConstraint,
                     VisibilityById,
                     vtkUnsignedCharArray);

//----------------------------------------------------------------------------
vtkStructuredVisibilityConstraint::vtkStructuredVisibilityConstraint()
{
  this->VisibilityById = 0;
  for (int i=0; i<3; i++)
    {
    this->Dimensions[i] = 0;
    }
  this->NumberOfIds = 0;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkStructuredVisibilityConstraint::~vtkStructuredVisibilityConstraint()
{
  if (this->VisibilityById)
    {
    this->VisibilityById->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkStructuredVisibilityConstraint::DeepCopy(
  vtkStructuredVisibilityConstraint* src)
{
  memcpy(this->Dimensions, src->Dimensions, 3*sizeof(int));
  // use vtkIdType to avoid 32-bit overflow
  this->NumberOfIds =
    static_cast<vtkIdType>(this->Dimensions[0])*
    static_cast<vtkIdType>(this->Dimensions[1])*
    static_cast<vtkIdType>(this->Dimensions[2]);
  if(src->VisibilityById)
    {
    if (!this->VisibilityById)
      {
      this->VisibilityById = vtkUnsignedCharArray::New();
      }
    this->VisibilityById->DeepCopy(src->VisibilityById);
    }
  this->Initialized = src->Initialized;
}

//----------------------------------------------------------------------------
void vtkStructuredVisibilityConstraint::ShallowCopy(
  vtkStructuredVisibilityConstraint* src)
{
  memcpy(this->Dimensions, src->Dimensions, 3*sizeof(int));
  this->NumberOfIds =
    static_cast<vtkIdType>(this->Dimensions[0])*
    static_cast<vtkIdType>(this->Dimensions[1])*
    static_cast<vtkIdType>(this->Dimensions[2]);
  this->SetVisibilityById(src->VisibilityById);
  this->Initialized = src->Initialized;
}

//----------------------------------------------------------------------------
void vtkStructuredVisibilityConstraint::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VisibilityById: ";
  if (this->VisibilityById)
    {
    os << endl;
    this->VisibilityById->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Dimensions: "
     << this->Dimensions[0] << " "
     << this->Dimensions[1] << " "
     << this->Dimensions[2]
     << endl;
}

