/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayCoordinateIterator.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayCoordinateIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/iterator>

vtkStandardNewMacro(vtkArrayCoordinateIterator);

vtkArrayCoordinateIterator::vtkArrayCoordinateIterator() :
  Current(0),
  End(0)
{
}

vtkArrayCoordinateIterator::~vtkArrayCoordinateIterator()
{
}

void vtkArrayCoordinateIterator::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << "Extents:";
  for(vtkIdType i = 0; i != this->Extents.GetDimensions(); ++i)
    os << " " << this->Extents[i];
  os << endl;
  
  os << "Indices:";
  for(vtkIdType i = 0; i != this->Coordinates.GetDimensions(); ++i)
    os << " " << this->Coordinates[i];
  os << endl;
  
  os << "Current: " << this->Current << endl;
  os << "End: " << this->End << endl;
}

void vtkArrayCoordinateIterator::SetExtents(const vtkArrayExtents& extents)
{
  this->Extents = extents;
  this->Coordinates.SetDimensions(extents.GetDimensions());
  this->Current = 0;
  this->End = extents.GetSize();
}

bool vtkArrayCoordinateIterator::HasNext()
{
  return this->Current < this->End;
}

vtkArrayCoordinates vtkArrayCoordinateIterator::Next()
{
  vtkArrayCoordinates result = this->Coordinates;

  for(vtkIdType i = this->Extents.GetDimensions() - 1; i >= 0; --i)
    {
    if(++this->Coordinates[i] < this->Extents[i])
      break;
      
    this->Coordinates[i] = 0;
    }

  ++this->Current;
  return result;
}

