/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.cxx
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
#include "vtkPointDataToCellData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPointDataToCellData, "1.21");
vtkStandardNewMacro(vtkPointDataToCellData);

// Instantiate object so that point data is not passed to output.
vtkPointDataToCellData::vtkPointDataToCellData()
{
  this->PassPointData = 0;
}

void vtkPointDataToCellData::Execute()
{
  vtkIdType cellId, ptId;
  vtkIdType numCells, numPts;
  vtkDataSet *input= this->GetInput();
  vtkDataSet *output= this->GetOutput();
  vtkPointData *inPD=input->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int maxCellSize=input->GetMaxCellSize();
  vtkIdList *cellPts;
  float weight;
  float *weights;

  vtkDebugMacro(<<"Mapping point data to cell data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numCells=input->GetNumberOfCells()) < 1 )
    {
    vtkDebugMacro(<<"No input cells!");
    return;
    }
  weights=new float[maxCellSize];
  
  cellPts = vtkIdList::New();
  cellPts->Allocate(maxCellSize);

  // Pass the cell data first. The fields and attributes
  // which also exist in the point data of the input will
  // be over-written during CopyAllocate
  output->GetCellData()->PassData(input->GetCellData());

  // notice that inPD and outCD are vtkPointData and vtkCellData; respectively.
  // It's weird, but it works.
  outCD->CopyAllocate(inPD,numCells);

  int abort=0;
  vtkIdType progressInterval=numCells/20 + 1;
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress((float)cellId/numCells);
      abort = GetAbortExecute();
      }

    input->GetCellPoints(cellId, cellPts);
    numPts = cellPts->GetNumberOfIds();
    if ( numPts > 0 )
      {
      weight = 1.0 / numPts;
      for (ptId=0; ptId < numPts; ptId++)
        {
        weights[ptId] = weight;
        }
      outCD->InterpolatePoint(inPD, cellId, cellPts, weights);
      }
    }

  if ( this->PassPointData )
    {
    output->GetPointData()->PassData(input->GetPointData());
    }
  
  cellPts->Delete();
  delete [] weights;
}

void vtkPointDataToCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
}
