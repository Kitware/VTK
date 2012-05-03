// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridVolumeRayCastIterator.h"


//----------------------------------------------------------------------------

vtkUnstructuredGridVolumeRayCastIterator::vtkUnstructuredGridVolumeRayCastIterator()
{
  this->Bounds[0] = 0.0;
  this->Bounds[1] = 1.0;

  this->MaxNumberOfIntersections = 32;
}

vtkUnstructuredGridVolumeRayCastIterator::~vtkUnstructuredGridVolumeRayCastIterator()
{
}

void vtkUnstructuredGridVolumeRayCastIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: [" << this->Bounds[0] << ", " << this->Bounds[1]
     << "]" << endl;
  os << indent << "MaxNumberOfIntersections: "
     << this->MaxNumberOfIntersections << endl;
}
