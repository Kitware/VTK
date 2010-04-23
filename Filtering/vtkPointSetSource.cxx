/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSetSource.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"


//----------------------------------------------------------------------------
vtkPointSetSource::vtkPointSetSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPointSet *vtkPointSetSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkPointSet *>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkPointSet *vtkPointSetSource::GetOutput(int idx)
{
  return static_cast<vtkPointSet *>(this->vtkSource::GetOutput(idx));
}

//----------------------------------------------------------------------------
void vtkPointSetSource::SetOutput(vtkPointSet *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
int vtkPointSetSource::FillOutputPortInformation(int port,
                                                 vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPointSetSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
