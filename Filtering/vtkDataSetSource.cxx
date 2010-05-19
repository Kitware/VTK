/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetSource.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"


vtkDataSetSource::vtkDataSetSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkDataSetSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkDataSet *>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkDataSetSource::SetOutput(vtkDataSet *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkDataSetSource::GetOutput(int idx)
{
  return static_cast<vtkDataSet *>( this->vtkSource::GetOutput(idx) ); 
}

//----------------------------------------------------------------------------
int vtkDataSetSource::FillOutputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
