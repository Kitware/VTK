/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxSource.h"

#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxSource, "1.2");

//----------------------------------------------------------------------------
vtkHierarchicalBoxSource::vtkHierarchicalBoxSource()
{
  this->vtkSource::SetNthOutput(0, vtkHierarchicalBoxDataSet::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet *vtkHierarchicalBoxSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkHierarchicalBoxDataSet *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet *vtkHierarchicalBoxSource::GetOutput(int idx)
{
  return (vtkHierarchicalBoxDataSet *) this->vtkSource::GetOutput(idx); 
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxSource::SetOutput(vtkHierarchicalBoxDataSet *output)
{
  this->vtkSource::SetNthOutput(0, output);
}


//----------------------------------------------------------------------------
void vtkHierarchicalBoxSource::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces, ghostLevel;
  vtkHierarchicalBoxDataSet *output = (vtkHierarchicalBoxDataSet *)data;
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
void vtkHierarchicalBoxSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
