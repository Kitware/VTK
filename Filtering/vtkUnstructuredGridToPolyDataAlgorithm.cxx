/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToPolyDataAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridToPolyDataAlgorithm.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkUnstructuredGridToPolyDataAlgorithm, "1.1");
vtkStandardNewMacro(vtkUnstructuredGridToPolyDataAlgorithm);

//----------------------------------------------------------------------------
int vtkUnstructuredGridToPolyDataAlgorithm::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridToPolyDataAlgorithm::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
