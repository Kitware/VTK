/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataPiece.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkExtractPolyDataPiece.h"
#include "vtkObjectFactory.h"
#include "vtkOBBDicer.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
vtkExtractPolyDataPiece* vtkExtractPolyDataPiece::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractPolyDataPiece");
  if(ret)
    {
    return (vtkExtractPolyDataPiece*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractPolyDataPiece;
}

vtkExtractPolyDataPiece::vtkExtractPolyDataPiece()
{
  this->CreateGhostCells = 1;
}

void vtkExtractPolyDataPiece::ComputeInputUpdateExtents(vtkDataObject *out)
{
  vtkPolyData *input = this->GetInput();
  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }

  out = out;
  input->SetUpdateExtent(0, 1, 0);
}

void vtkExtractPolyDataPiece::ExecuteInformation()
{
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("Missing input");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}
  
void vtkExtractPolyDataPiece::ComputeCellTags(vtkIntArray *tags, 
					      vtkIdList *pointOwnership,
					      int piece, int numPieces)
{
  vtkPolyData *input;
  vtkIdType idx, j, numCells, ptId;
  vtkIdList *cellPtIds;

  input = this->GetInput();
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
    if ((idx * numPieces / numCells) == piece)
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
  
  //dicer->SetInput(input);
  //dicer->SetDiceModeToSpecifiedNumberOfPieces();
  //dicer->SetNumberOfPieces(numPieces);
  //dicer->Update();
  
  //intermediate->ShallowCopy(dicer->GetOutput());
  //intermediate->BuildLinks();
  //pointScalars = intermediate->GetPointData()->GetScalars();
}

void vtkExtractPolyDataPiece::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  vtkIntArray *cellTags;
  int ghostLevel, piece, numPieces;
  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList *newCellPts = vtkIdList::New();
  vtkIdList *pointOwnership;
  vtkCell *cell;
  vtkPoints *newPoints;
  vtkUnsignedCharArray* cellGhostLevels = 0;
  vtkUnsignedCharArray* pointGhostLevels = 0;
  vtkIdType ptId, newId, numPts, i;
  int numCellPts;
  float *x;

  // Pipeline update piece will tell us what to generate.
  ghostLevel = output->GetUpdateGhostLevel();
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  
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
  // Cell tags end up bieing 0 for cells in piece and -1 for all others.
  // Point ownership is the cell that owns the point.
  this->ComputeCellTags(cellTags, pointOwnership, piece, numPieces);
  
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

}

void vtkExtractPolyDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
}

void vtkExtractPolyDataPiece::AddGhostLevel(vtkPolyData *input,
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
