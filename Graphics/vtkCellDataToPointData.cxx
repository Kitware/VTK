/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.cxx
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
#include "vtkCellDataToPointData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCellDataToPointData, "1.22");
vtkStandardNewMacro(vtkCellDataToPointData);

// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
}

#define VTK_MAX_CELLS_PER_POINT 4096

void vtkCellDataToPointData::Execute()
{
  vtkIdType cellId, ptId;
  vtkIdType numCells, numPts;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output= this->GetOutput();
  vtkCellData *inPD=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkIdList *cellIds;
  float weight;
  float *weights;

  vtkDebugMacro(<<"Mapping cell data to point data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  cellIds = vtkIdList::New();
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No input point data!");
    cellIds->Delete();
    return;
    }
  weights = new float[VTK_MAX_CELLS_PER_POINT];
  
  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->PassData(input->GetPointData());

  // notice that inPD and outPD are vtkCellData and vtkPointData; respectively.
  // It's weird, but it works.
  outPD->CopyAllocate(inPD,numPts);

  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((float)ptId/numPts);
      abort = GetAbortExecute();
      }

    input->GetPointCells(ptId, cellIds);
    numCells = cellIds->GetNumberOfIds();
    if ( numCells > 0 )
      {
      weight = 1.0 / numCells;
      for (cellId=0; cellId < numCells; cellId++)
        {
        weights[cellId] = weight;
        }
      outPD->InterpolatePoint(inPD, ptId, cellIds, weights);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }

  if ( this->PassCellData )
    {
    output->GetCellData()->PassData(input->GetCellData());
    }

  cellIds->Delete();
  delete [] weights;
}

void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
}
