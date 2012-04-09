/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUnstructuredGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractUnstructuredGridPiece.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractUnstructuredGridPiece);

vtkExtractUnstructuredGridPiece::vtkExtractUnstructuredGridPiece()
{
  this->CreateGhostCells = 1;
}

int vtkExtractUnstructuredGridPiece::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              0);
  return 1;
}

int vtkExtractUnstructuredGridPiece::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  return 1;
}
  
void vtkExtractUnstructuredGridPiece::ComputeCellTags(vtkIntArray *tags, 
                                              vtkIdList *pointOwnership,
                                              int piece, int numPieces,
                                              vtkUnstructuredGrid *input)
{
  int j;
  vtkIdType idx, numCells, ptId;
  vtkIdType* cellPointer;
  vtkIdType* ids;
  vtkIdType numCellPts;

  numCells = input->GetNumberOfCells();
  
  // Clear Point ownership.  This is only necessary if we
  // Are creating ghost points.
  if (pointOwnership)
    {
    for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
      {
      pointOwnership->SetId(idx, -1);
      }
    }
    
  // Brute force division.
  cellPointer = (input->GetCells() ? input->GetCells()->GetPointer() : 0);
  for (idx = 0; idx < numCells; ++idx)
    {
    if ((idx * numPieces / numCells) == piece)
      {
      tags->SetValue(idx, 0);
      }
    else
      {
      tags->SetValue(idx, -1);
      }
    // Fill in point ownership mapping.
    if (pointOwnership)
      {
      numCellPts = cellPointer[0];
      ids = cellPointer+1;
      // Move to the next cell.
      cellPointer += (1 + numCellPts);
      for (j = 0; j < numCellPts; ++j)
        {
        ptId = ids[j];
        if (pointOwnership->GetId(ptId) == -1)
          {
          pointOwnership->SetId(ptId, idx);
          }
        }
      }
    }
}

int vtkExtractUnstructuredGridPiece::RequestData(
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
  unsigned char* cellTypes = (input->GetCellTypesArray() ? input->GetCellTypesArray()->GetPointer(0) : 0);
  int cellType;
  vtkIntArray *cellTags;
  int ghostLevel, piece, numPieces;
  vtkIdType cellId, newCellId;
  vtkIdList *pointMap;
  vtkIdList *newCellPts = vtkIdList::New();
  vtkPoints *newPoints;
  vtkUnsignedCharArray* cellGhostLevels = 0;
  vtkIdList *pointOwnership = 0;
  vtkUnsignedCharArray* pointGhostLevels = 0;
  vtkIdType i, ptId, newId, numPts, numCells;
  int numCellPts;
  vtkIdType *cellPointer;
  vtkIdType *ids;
  double *x;

  // Pipeline update piece will tell us what to generate.
  ghostLevel = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  piece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  
  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  if (ghostLevel > 0 && this->CreateGhostCells)
    {
    cellGhostLevels = vtkUnsignedCharArray::New();
    cellGhostLevels->Allocate(numCells);
    // We may want to create point ghost levels even
    // if there are no ghost cells.  Since it cost extra,
    // and no filter really uses it, and the filter did not
    // create a point ghost level array for this case before,
    // I will leave it the way it was.
    pointOwnership = vtkIdList::New();
    pointOwnership->Allocate(numPts);
    pointGhostLevels = vtkUnsignedCharArray::New();
    pointGhostLevels->Allocate(numPts);
    }
    
  // Break up cells based on which piece they belong to.
  cellTags = vtkIntArray::New();
  cellTags->Allocate(input->GetNumberOfCells(), 1000);
  // Cell tags end up being 0 for cells in piece and -1 for all others.
  // Point ownership is the cell that owns the point.
  this->ComputeCellTags(cellTags, pointOwnership, piece, numPieces, input);
  
  // Find the layers of ghost cells.
  if (this->CreateGhostCells)
    {
    for (i = 0; i < ghostLevel; i++)
      {
      this->AddGhostLevel(input, cellTags, i+1);
      }
    }
  
  // Filter the cells.

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
  cellPointer = (input->GetCells() ? input->GetCells()->GetPointer() : 0);
  for (cellId=0; cellId < numCells; cellId++)
    {
    // Direct access to cells.
    cellType = cellTypes[cellId];
    numCellPts = cellPointer[0];
    ids = cellPointer+1;
    // Move to the next cell.
    cellPointer += (1 + *cellPointer);

    if ( cellTags->GetValue(cellId) != -1) // satisfied thresholding
      {
      if (cellGhostLevels)
        {                
        cellGhostLevels->InsertNextValue(
          (unsigned char)(cellTags->GetValue(cellId)));
        }
     
      for (i=0; i < numCellPts; i++)
        {
        ptId = ids[i];
        if ( (newId = pointMap->GetId(ptId)) < 0 )
          {
          x = input->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          if (pointGhostLevels && pointOwnership)
            {
            pointGhostLevels->InsertNextValue(
              cellTags->GetValue(pointOwnership->GetId(ptId)));
            }
          pointMap->SetId(ptId,newId);
          outPD->CopyData(pd,ptId,newId);
          }
        newCellPts->InsertId(i,newId);
        }
      newCellId = output->InsertNextCell(cellType,newCellPts);
      outCD->CopyData(cd,cellId,newCellId);
      newCellPts->Reset();
      } // satisfied thresholding
    } // for all cells
  
  // Split up points that are not used by cells, 
  // and have not been assigned to any piece. 
  // Count the number of unassigned points.  This is an extra pass through
  // the points, but the pieces will be better load balanced and
  // more spatially coherent.
  vtkIdType count = 0;
  vtkIdType idx;
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
    {
    if (pointMap->GetId(idx) == -1)
      {
      ++count;
      }
    }
  vtkIdType count2 = 0;
  for (idx = 0; idx < input->GetNumberOfPoints(); ++idx)
    {
    if (pointMap->GetId(idx) == -1)
      {
      if ((count2++ * numPieces / count) == piece)
        {
        x = input->GetPoint(idx);
        newId = newPoints->InsertNextPoint(x);
        if (pointGhostLevels)
          {
          pointGhostLevels->InsertNextValue(0);
          }
        outPD->CopyData(pd,idx,newId);
        }
      }
    }  
  
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
  if (pointOwnership)
    {
    pointOwnership->Delete();
    pointOwnership = 0;
    }

  return 1;
}

void vtkExtractUnstructuredGridPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " 
     << (this->CreateGhostCells ? "On\n" : "Off\n");
}


// This method is still slow...
void vtkExtractUnstructuredGridPiece::AddGhostLevel(vtkUnstructuredGrid *input,
                                                    vtkIntArray *cellTags, 
                                                    int level)
{
  vtkIdType numCells, pointId, cellId, i;
  int j, k;
  vtkGenericCell *cell1 = vtkGenericCell::New();
  vtkGenericCell *cell2 = vtkGenericCell::New();
  vtkIdList *cellIds = vtkIdList::New();
  
  numCells = input->GetNumberOfCells();
  
  for (i = 0; i < numCells; i++)
    {
    if (cellTags->GetValue(i) == level - 1)
      {
      input->GetCell(i, cell1);
      for (j = 0; j < cell1->GetNumberOfPoints(); j++)
        {
        pointId = cell1->GetPointId(j);
        input->GetPointCells(pointId, cellIds);
        for (k = 0; k < cellIds->GetNumberOfIds(); k++)
          {
          cellId = cellIds->GetId(k);
          if (cellTags->GetValue(cellId) == -1)
            {
            input->GetCell(cellId, cell2);
            cellTags->SetValue(cellId, level);
            }
          }
        }
      }
    }
  cell1->Delete();
  cell2->Delete();
  cellIds->Delete();
}
