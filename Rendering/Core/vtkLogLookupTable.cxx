/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogLookupTable.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkStandardNewMacro(vtkLogLookupTable);

// Construct with (minimum,maximum) range 1 to 10 (based on
// logarithmic values).
vtkLogLookupTable::vtkLogLookupTable(int sze, int ext)
  : vtkLookupTable(sze,ext)
{
  this->Scale = VTK_SCALE_LOG10;

  this->TableRange[0] = 1;
  this->TableRange[1] = 10;
}

void vtkLogLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
