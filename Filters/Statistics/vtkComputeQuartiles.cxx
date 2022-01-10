/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeQuartiles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkComputeQuartiles.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkComputeQuartiles);
//------------------------------------------------------------------------------
vtkComputeQuartiles::vtkComputeQuartiles()
{
  //
  // Note:
  // Implementation of this class has moved to new super class vtkComputeNtiles.
  // This class is kept for backwards compatibility.
  //

  this->SetNumberOfIntervals(4);
}

//------------------------------------------------------------------------------
void vtkComputeQuartiles::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
