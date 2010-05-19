/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToUnstructuredGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToUnstructuredGridFilter.h"

#include "vtkInformation.h"
#include "vtkUnstructuredGrid.h"


//----------------------------------------------------------------------------
vtkDataSetToUnstructuredGridFilter::vtkDataSetToUnstructuredGridFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkDataSetToUnstructuredGridFilter::~vtkDataSetToUnstructuredGridFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToUnstructuredGridFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToUnstructuredGridFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkDataSet *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int
vtkDataSetToUnstructuredGridFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetToUnstructuredGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
