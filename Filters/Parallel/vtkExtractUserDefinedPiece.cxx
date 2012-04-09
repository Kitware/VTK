/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUserDefinedPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkExtractUserDefinedPiece.h"

#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkExtractUserDefinedPiece);

vtkExtractUserDefinedPiece::vtkExtractUserDefinedPiece()
{
  this->ConstantData = NULL;
  this->ConstantDataLen = 0;
  this->InPiece = NULL;
}
vtkExtractUserDefinedPiece::~vtkExtractUserDefinedPiece()
{
  if (this->ConstantData){
    delete [] (char *)this->ConstantData;
    this->ConstantData = NULL;
  }
}
void vtkExtractUserDefinedPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "ConstantData: " << this->ConstantData
     << indent << "ConstantDataLen: " << this->ConstantDataLen
     << indent << "InPiece: " << this->InPiece
     << "\n";
}
void vtkExtractUserDefinedPiece::SetConstantData(void *data, int len)
{
  this->ConstantData = new char [len];
  this->ConstantDataLen = len;

  memcpy(this->ConstantData, data, len);

  this->Modified();
}

int vtkExtractUserDefinedPiece::GetConstantData(void **data)
{
  *data = this->ConstantData;
  return this->ConstantDataLen;
}


// This is exactly vtkExtractUnstructuredGridPiece::Execute(), with 
// the exception that we call ComputeCellTagsWithFunction rather
// than ComputeCellTags.  If ComputeCellTags were virtual, we could
// just override it here.

int vtkExtractUserDefinedPiece::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  vtkIntArray *cellTags;
  int ghostLevel;
  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts = vtkIdList::New();
  vtkIdList *pointOwnership;
  vtkCell *cell;
  vtkPoints *newPoints;
  vtkUnsignedCharArray* cellGhostLevels = 0;
  vtkUnsignedCharArray* pointGhostLevels = 0;
  vtkIdType i, ptId, newId, numPts;
  int numCellPts;
  double *x;

  // Pipeline update piece will tell us what to generate.
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  if (ghostLevel > 0 && this->CreateGhostCells)
    {
    cellGhostLevels = vtkUnsignedCharArray::New();
    pointGhostLevels = vtkUnsignedCharArray::New();
    cellGhostLevels->Allocate(input->GetNumberOfCells());
    pointGhostLevels->Allocate(input->GetNumberOfPoints());
    }

  // Break up cells based on which piece they belong to.
  cellTags = vtkIntArray::New();
  cellTags->Allocate(input->GetNumberOfCells(), 1000);
  pointOwnership = vtkIdList::New();
  pointOwnership->Allocate(input->GetNumberOfPoints());

  // Cell tags end up being 0 for cells in piece and -1 for all others.
  // Point ownership is the cell that owns the point.

  this->ComputeCellTagsWithFunction(cellTags, pointOwnership, input);

  // Find the layers of ghost cells.
  if (this->CreateGhostCells)
    {
    for (i = 0; i < ghostLevel; i++)
      {
      this->AddGhostLevel(input, cellTags, i+1);
      }
    }

  // Filter the cells.

  numPts = input->GetNumberOfPoints();
  output->Allocate(input->GetNumberOfCells());
  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);

  pointMap = vtkIdList::New(); //maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i=0; i < numPts; i++)
    {
    pointMap->SetId(i,-1);
    }

  // Filter the cells
  for (cellId=0; cellId < input->GetNumberOfCells(); cellId++)
    {
    if ( cellTags->GetValue(cellId) != -1) // satisfied thresholding
      {
      if (cellGhostLevels)
        {
        cellGhostLevels->InsertNextValue(
          (unsigned char)(cellTags->GetValue(cellId)));
        }

      cell = input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      numCellPts = cell->GetNumberOfPoints();

      for (i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          x = input->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          if (pointGhostLevels)
            {
            pointGhostLevels->InsertNextValue(
              cellTags->GetValue(pointOwnership->GetId(ptId)));
            }
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          }
        newCellPts->InsertId(i,newId);
        }
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells
  
  vtkDebugMacro(<< "Extracted " << output->GetNumberOfCells() 
                << " number of cells.");
  
  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();
      
  if (cellGhostLevels)
    { 
    cellGhostLevels->SetName("vtkGhostLevels");
    output->GetCellData()->AddArray(cellGhostLevels);
    cellGhostLevels->Delete();
    cellGhostLevels = 0;
     }
  if (pointGhostLevels)
    {
    pointGhostLevels->SetName("vtkGhostLevels");
    output->GetPointData()->AddArray(pointGhostLevels);
    pointGhostLevels->Delete();
    pointGhostLevels = 0;
    } 
  output->SetPoints(newPoints);
  newPoints->Delete();
    
  output->Squeeze();
  cellTags->Delete();
  pointOwnership->Delete();

  return 1;
}
void vtkExtractUserDefinedPiece::
ComputeCellTagsWithFunction(vtkIntArray *tags,
                            vtkIdList *pointOwnership,
                            vtkUnstructuredGrid *input)
{
  int j;
  vtkIdType idx, numCells, ptId;
  vtkIdList *cellPtIds;

  numCells = input->GetNumberOfCells();

  cellPtIds = vtkIdList::New();
  // Clear Point ownership.
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
    {
    pointOwnership->SetId(idx, -1);
    }

  // Brute force division.
  for (idx = 0; idx < numCells; ++idx)
    {
    if (this->InPiece(idx, input, this->ConstantData))
      {
      tags->SetValue(idx, 0);
      }
    else
      {
      tags->SetValue(idx, -1);
      }
    // Fill in point ownership mapping.
    input->GetCellPoints(idx, cellPtIds);
    for (j = 0; j < cellPtIds->GetNumberOfIds(); ++j)
      {
      ptId = cellPtIds->GetId(j);
      if (pointOwnership->GetId(ptId) == -1)
        {
        pointOwnership->SetId(ptId, idx);
        }
      }
    }

  cellPtIds->Delete();
}

