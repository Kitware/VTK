/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellCenters.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"

vtkStandardNewMacro(vtkCellCenters);

//----------------------------------------------------------------------------
// Construct object with vertex cell generation turned off.
vtkCellCenters::vtkCellCenters()
{
  this->VertexCells = 0;
}

//----------------------------------------------------------------------------
// Generate points
int vtkCellCenters::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, numCells;
  int subId;
  vtkCellData *inCD;
  vtkPointData *outPD;
  vtkPoints *newPts;
  vtkCell *cell;
  double x[3], pcoords[3];
  double *weights;

  inCD=input->GetCellData();
  outPD=output->GetPointData();

  if ( (numCells = input->GetNumberOfCells()) < 1 )
    {
    vtkDebugMacro(<<"No cells to generate center points for");
    return 1;
    }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numCells);
  weights = new double [input->GetMaxCellSize()];

  int abort=0;
  vtkIdType progressInterval = numCells/10 + 1;
  int hasEmptyCells = 0;
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( ! (cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Processing #" << cellId);
      this->UpdateProgress (0.5*cellId/numCells);
      abort = this->GetAbortExecute();
      }

    cell = input->GetCell(cellId);
    if (cell->GetCellType() != VTK_EMPTY_CELL)
      {
      subId = cell->GetParametricCenter(pcoords);
      cell->EvaluateLocation(subId, pcoords, x, weights);
      newPts->SetPoint(cellId,x);
      }
    else
      {
      hasEmptyCells = 1;
      }
    }

  if ( this->VertexCells )
    {
    vtkIdType pts[1];
    vtkCellData *outCD=output->GetCellData();
    vtkCellArray *verts = vtkCellArray::New();
    verts->Allocate(verts->EstimateSize(1,numCells),1);

    for (cellId=0; cellId < numCells && !abort; cellId++)
      {
      if ( ! (cellId % progressInterval) )
        {
        vtkDebugMacro(<<"Processing #" << cellId);
        this->UpdateProgress (0.5+0.5*cellId/numCells);
        abort = this->GetAbortExecute();
        }

      cell = input->GetCell(cellId);
      if (cell->GetCellType() != VTK_EMPTY_CELL)
        {
        pts[0] = cellId;
        verts->InsertNextCell(1,pts);
        }
      }

    output->SetVerts(verts);
    verts->Delete();
    if (!hasEmptyCells)
      {
      outCD->PassData(inCD); //only if verts are generated
      }
    }

  // clean up and update output
  output->SetPoints(newPts);
  newPts->Delete();

  if (!hasEmptyCells)
    {
    outPD->PassData(inCD); //because number of points = number of cells
    }
  delete [] weights;

  return 1;
}

//----------------------------------------------------------------------------
int vtkCellCenters::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}

