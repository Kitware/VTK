/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellDataToPointData.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkCellDataToPointData);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->PassCellData = 0;
}

#define VTK_MAX_CELLS_PER_POINT 4096

//----------------------------------------------------------------------------
int vtkCellDataToPointData::RequestData(
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
  vtkCellData *inPD=input->GetCellData();
  vtkPointData *outPD=output->GetPointData();
  vtkIdList *cellIds;
  double weight;
  double *weights;

  vtkDebugMacro(<<"Mapping cell data to point data");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  cellIds = vtkIdList::New();
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
    {
    vtkDebugMacro(<<"No input point data!");
    cellIds->Delete();
    return 1;
    }
  weights = new double[VTK_MAX_CELLS_PER_POINT];
  
  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->CopyGlobalIdsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->CopyFieldOff("vtkGhostLevels");

  // notice that inPD and outPD are vtkCellData and vtkPointData; respectively.
  // It's weird, but it works.
  outPD->InterpolateAllocate(inPD,numPts);

  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
      }

    input->GetPointCells(ptId, cellIds);
    numCells = cellIds->GetNumberOfIds();
    if ( numCells > 0 && numCells < VTK_MAX_CELLS_PER_POINT )
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

  if ( !this->PassCellData )
    {
    output->GetCellData()->CopyAllOff();
    output->GetCellData()->CopyFieldOn("vtkGhostLevels");
    }
  output->GetCellData()->PassData(input->GetCellData());

  cellIds->Delete();
  delete [] weights;
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
}
