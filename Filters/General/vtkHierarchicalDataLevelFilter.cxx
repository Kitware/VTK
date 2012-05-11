/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataLevelFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataLevelFilter.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkHierarchicalDataLevelFilter);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkHierarchicalDataLevelFilter::vtkHierarchicalDataLevelFilter()
{
}

vtkHierarchicalDataLevelFilter::~vtkHierarchicalDataLevelFilter()
{
}

void vtkHierarchicalDataLevelFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
