/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToPolyDataAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridToPolyDataAlgorithm.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkRectilinearGridToPolyDataAlgorithm, "1.1");
vtkStandardNewMacro(vtkRectilinearGridToPolyDataAlgorithm);

//----------------------------------------------------------------------------
int vtkRectilinearGridToPolyDataAlgorithm::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkRectilinearGridToPolyDataAlgorithm::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
