/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetToPolyDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSetToPolyDataFilter.h"

#include "vtkGenericDataSet.h"

#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
#include "vtkInformation.h"
#endif

vtkCxxRevisionMacro(vtkGenericDataSetToPolyDataFilter, "1.3");

vtkGenericDataSetToPolyDataFilter
::vtkGenericDataSetToPolyDataFilter()
{
#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
  this->SetNumberOfInputPorts(1);
#endif
  
  this->NumberOfRequiredInputs = 1;
}

//----------------------------------------------------------------------------
void vtkGenericDataSetToPolyDataFilter::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkGenericDataSetToPolyDataFilter::SetInput(vtkGenericDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkGenericDataSet *vtkGenericDataSetToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkGenericDataSet *>(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkGenericDataSetToPolyDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input == NULL)
    {
    return;
    }
  
  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);
  input->RequestExactExtentOn();
}

#if VTK_MAJOR_VERSION>4 || (VTK_MAJOR_VERSION==4 && VTK_MINOR_VERSION>4)
//----------------------------------------------------------------------------
int vtkGenericDataSetToPolyDataFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  return 1;
}
#endif
