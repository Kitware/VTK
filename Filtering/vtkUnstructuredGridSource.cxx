/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridSource.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"


//----------------------------------------------------------------------------
vtkUnstructuredGridSource::vtkUnstructuredGridSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);

  this->vtkSource::SetNthOutput(0, vtkUnstructuredGrid::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid *vtkUnstructuredGridSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkUnstructuredGrid *>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridSource::SetOutput(vtkUnstructuredGrid *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridSource::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces, ghostLevel;
  vtkUnstructuredGrid *output = static_cast<vtkUnstructuredGrid *>(data);
  int idx;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);
    
  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return;
    }
  
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->SetUpdateExtent(piece, numPieces, ghostLevel);
      }
    }
}

vtkUnstructuredGrid *vtkUnstructuredGridSource::GetOutput(int idx)
{
  return static_cast<vtkUnstructuredGrid *>( this->vtkSource::GetOutput(idx) ); 
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridSource::FillOutputPortInformation(int port,
                                                         vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
