/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridToPolyDataFilter.h"

#include "vtkInformation.h"
#include "vtkRectilinearGrid.h"


//----------------------------------------------------------------------------
vtkRectilinearGridToPolyDataFilter::vtkRectilinearGridToPolyDataFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkRectilinearGridToPolyDataFilter::~vtkRectilinearGridToPolyDataFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkRectilinearGridToPolyDataFilter::SetInput(vtkRectilinearGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkRectilinearGrid *vtkRectilinearGridToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkRectilinearGrid *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int
vtkRectilinearGridToPolyDataFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkRectilinearGridToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
