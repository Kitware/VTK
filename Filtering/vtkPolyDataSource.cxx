/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataSource.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"


//----------------------------------------------------------------------------
vtkPolyDataSource::vtkPolyDataSource()
{
  // A source has no inputs by default.
  this->SetNumberOfInputPorts(0);
  this->vtkSource::SetNthOutput(0, vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkPolyData *>(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataSource::GetOutput(int idx)
{
  return static_cast<vtkPolyData *>(this->vtkSource::GetOutput(idx));
}

//----------------------------------------------------------------------------
void vtkPolyDataSource::SetOutput(vtkPolyData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
void vtkPolyDataSource::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces, ghostLevel;
  vtkPolyData *output = static_cast<vtkPolyData *>(data);
  int idx;

  output->GetUpdateExtent(piece, numPieces, ghostLevel);
  
  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return;
    }
  
  if (ghostLevel < 0)
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

//----------------------------------------------------------------------------
int vtkPolyDataSource::FillOutputPortInformation(int port,
                                                 vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
