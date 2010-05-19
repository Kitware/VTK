/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectSource.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"


vtkDataObjectSource::vtkDataObjectSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);

  this->SetOutput(vtkDataObject::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}


//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return this->Outputs[0];
}

//----------------------------------------------------------------------------
void vtkDataObjectSource::SetOutput(vtkDataObject *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
int vtkDataObjectSource::FillOutputPortInformation(int port,
                                                   vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataObjectSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
