/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredGridSource.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"


//----------------------------------------------------------------------------
vtkStructuredGridSource::vtkStructuredGridSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);

  this->vtkSource::SetNthOutput(0, vtkStructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkStructuredGrid *vtkStructuredGridSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkStructuredGrid *>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkStructuredGrid *vtkStructuredGridSource::GetOutput(int idx)
{
  return static_cast<vtkStructuredGrid *>( this->vtkSource::GetOutput(idx) ); 
}

//----------------------------------------------------------------------------
void vtkStructuredGridSource::SetOutput(vtkStructuredGrid *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
int vtkStructuredGridSource::FillOutputPortInformation(int port,
                                                       vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkStructuredGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
