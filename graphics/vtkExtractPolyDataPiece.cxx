/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataPiece.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
#include "vtkGhostLevels.h"

//------------------------------------------------------------------------------
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

void vtkExtractPolyDataPiece::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *intermediate = vtkPolyData::New();
  vtkOBBDicer *dicer = vtkOBBDicer::New();
  vtkCellArray *newPolys = vtkCellArray::New();
  vtkScalars *pointScalars, *cellScalars = vtkScalars::New();
  int i, pointId, numCells;
  float scalar;
  vtkCell *cell;
  vtkGhostLevels *ghostLevels = vtkGhostLevels::New();
  int ghostLevel, piece, numPieces;
  
  ghostLevel = output->GetUpdateGhostLevel();
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  
  dicer->SetInput(input);
  dicer->SetDiceModeToSpecifiedNumberOfPieces();
  dicer->SetNumberOfPieces(numPieces);
  dicer->Update();
  
  intermediate->ShallowCopy(dicer->GetOutput());
  intermediate->BuildLinks();
  pointScalars = intermediate->GetPointData()->GetScalars();
  numCells = intermediate->GetNumberOfCells();
  
  for (i = 0; i < numCells; i++)
    {
    cell = intermediate->GetCell(i);
    pointId = cell->GetPointId(0);
    scalar = pointScalars->GetScalar(pointId);
    if (scalar == piece)
      {
      newPolys->InsertNextCell(cell);
      cellScalars->InsertScalar(i, 0);
      if (this->CreateGhostCells)
	{
	ghostLevels->InsertNextGhostLevel(0);
	}
      }
    else
      {
      cellScalars->InsertScalar(i, 100);
      }
    }
  
  if (this->CreateGhostCells)
    {
    for (i = 0; i < ghostLevel; i++)
      {
      this->AddGhostLevel(ghostLevels, intermediate, newPolys,
			  cellScalars, i+1);
      }
    }
  
  output->SetPolys(newPolys);
  output->SetPoints(intermediate->GetPoints());
  output->GetCellData()->SetScalars(cellScalars);
  if (this->CreateGhostCells)
    {
    output->GetCellData()->SetGhostLevels(ghostLevels);
    }
  dicer->Delete();
  cellScalars->Delete();
  newPolys->Delete();
  intermediate->Delete();
  ghostLevels->Delete();
}

void vtkExtractPolyDataPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  
  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");
}

void vtkExtractPolyDataPiece::AddGhostLevel(vtkGhostLevels *ghostLevels,
					    vtkPolyData *polyData,
					    vtkCellArray *newPolys,
					    vtkScalars *cellScalars,
					    int ghostLevel)
{
  int i, j, k, numCells, pointId, cellId;
  vtkGenericCell *cell1 = vtkGenericCell::New(), *cell2 = vtkGenericCell::New();
  vtkIdList *cellIds = vtkIdList::New();
  
  numCells = polyData->GetNumberOfCells();
  
  for (i = 0; i < numCells; i++)
    {
    if (cellScalars->GetScalar(i) == ghostLevel - 1)
      {
      polyData->GetCell(i, cell1);
      for (j = 0; j < cell1->GetNumberOfPoints(); j++)
        {
        pointId = cell1->GetPointId(j);
        polyData->GetPointCells(pointId, cellIds);
        for (k = 0; k < cellIds->GetNumberOfIds(); k++)
          {
          cellId = cellIds->GetId(k);
          if (cellScalars->GetScalar(cellId) == 100)
            {
            polyData->GetCell(cellId, cell2);
            newPolys->InsertNextCell(cell2);
            cellScalars->InsertScalar(cellId, ghostLevel);
            ghostLevels->InsertNextGhostLevel(ghostLevel);
            }
          }
        }
      }
    }
  cell1->Delete();
  cell2->Delete();
}
