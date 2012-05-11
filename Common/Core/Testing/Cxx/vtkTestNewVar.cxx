/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestNewVar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestNewVar.h"
#include "vtkPoints2D.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTestNewVar);

vtkTestNewVar::vtkTestNewVar()
{
}

vtkTestNewVar::~vtkTestNewVar()
{
}

vtkIdType vtkTestNewVar::GetPointsRefCount()
{
  // Note - this is valid until class destruction and then Delete() will be
  // called on the Data object, decrementing its reference count.
  return this->Points->GetReferenceCount();
}

vtkObject * vtkTestNewVar::GetPoints()
{
  return this->Points.GetPointer();
}

void vtkTestNewVar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Points: " << endl;
  this->Points->PrintSelf(os, indent.GetNextIndent());
}
