/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetToUnstructuredGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSetToUnstructuredGridFilter.h"

#include "vtkGenericDataSet.h"

vtkCxxRevisionMacro(vtkGenericDataSetToUnstructuredGridFilter, "1.1");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkGenericDataSetToUnstructuredGridFilter::SetInput(vtkGenericDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkGenericDataSet *vtkGenericDataSetToUnstructuredGridFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkGenericDataSet *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkGenericDataSetToUnstructuredGridFilter::PrintSelf(ostream& os,
                                                          vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
