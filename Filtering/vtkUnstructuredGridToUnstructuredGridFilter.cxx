/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToUnstructuredGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridToUnstructuredGridFilter.h"

#include "vtkInformation.h"
#include "vtkUnstructuredGrid.h"


// ----------------------------------------------------------------------------
vtkUnstructuredGridToUnstructuredGridFilter::vtkUnstructuredGridToUnstructuredGridFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

// ----------------------------------------------------------------------------
vtkUnstructuredGridToUnstructuredGridFilter::~vtkUnstructuredGridToUnstructuredGridFilter()
{
}

// ----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkUnstructuredGridToUnstructuredGridFilter::SetInput(
  vtkUnstructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

// ----------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid *vtkUnstructuredGridToUnstructuredGridFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkUnstructuredGrid *>(this->Inputs[0]);
}

// ----------------------------------------------------------------------------
int vtkUnstructuredGridToUnstructuredGridFilter::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

// ----------------------------------------------------------------------------
void vtkUnstructuredGridToUnstructuredGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
