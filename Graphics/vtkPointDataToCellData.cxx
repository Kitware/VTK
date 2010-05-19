/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointDataToCellData.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkPointDataToCellData);

//----------------------------------------------------------------------------
// Instantiate object so that point data is not passed to output.
vtkPointDataToCellData::vtkPointDataToCellData()
{
  this->PassPointData = 0;
}

//----------------------------------------------------------------------------
int vtkPointDataToCellData::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, ptId;
  vtkIdType numCells, numPts;
  vtkPointData *inPD=input->GetPointData();
  vtkCellData *outCD=output->GetCellData();
  int maxCellSize=input->GetMaxCellSize();
  vtkIdList *cellPts;
  double weight;
  double *weights;

  vtkDebugMacro(<<"Mapping point data to cell data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numCells=input->GetNumberOfCells()) < 1 )
    {
    vtkDebugMacro(<<"No input cells!");
    return 1;
    }
  weights=new double[maxCellSize];
  
  cellPts = vtkIdList::New();
  cellPts->Allocate(maxCellSize);

  // Pass the cell data first. The fields and attributes
  // which also exist in the point data of the input will
  // be over-written during CopyAllocate
  output->GetCellData()->CopyGlobalIdsOff();
  output->GetCellData()->PassData(input->GetCellData());
  output->GetCellData()->CopyFieldOff("vtkGhostLevels");

  // notice that inPD and outCD are vtkPointData and vtkCellData; respectively.
  // It's weird, but it works.
  outCD->InterpolateAllocate(inPD,numCells);

  int abort=0;
  vtkIdType progressInterval=numCells/20 + 1;
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress((double)cellId/numCells);
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

  if ( !this->PassPointData )
    {
    output->GetPointData()->CopyAllOff();
    output->GetPointData()->CopyFieldOn("vtkGhostLevels");
    }
  output->GetPointData()->PassData(input->GetPointData());

  cellPts->Delete();
  delete [] weights;

  return 1;
}

//----------------------------------------------------------------------------
void vtkPointDataToCellData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
}
