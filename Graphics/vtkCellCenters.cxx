/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.cxx
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
#include "vtkCellCenters.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCellCenters, "1.18");
vtkStandardNewMacro(vtkCellCenters);

// Construct object with vertex cell generation turned off.
vtkCellCenters::vtkCellCenters()
{
  this->VertexCells = 0;
}

// Generate points
void vtkCellCenters::Execute()
{
  vtkIdType cellId, numCells;
  int subId;
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkCellData *inCD;
  vtkPointData *outPD;
  vtkPoints *newPts;
  vtkCell *cell;
  float x[3], pcoords[3];
  float *weights = NULL;

  if (input == NULL)
    {
    vtkErrorMacro(<<"Input is NULL");
    return;
    }

  vtkDebugMacro(<<"Generating cell center points");

  inCD=input->GetCellData();
  outPD=output->GetPointData();

  if ( (numCells = input->GetNumberOfCells()) < 1 )
    {
    vtkWarningMacro(<<"No cells to generate center points for");
    return;
    }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numCells);
  weights = new float [input->GetMaxCellSize()];

  int abort=0;
  vtkIdType progressInterval = numCells/10 + 1;
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( ! (cellId % progressInterval) ) 
      {
      vtkDebugMacro(<<"Processing #" << cellId);
      this->UpdateProgress (0.5*cellId/numCells);
      abort = this->GetAbortExecute();
      }

    cell = input->GetCell(cellId);
    subId = cell->GetParametricCenter(pcoords);
    cell->EvaluateLocation(subId, pcoords, x, weights);
    newPts->SetPoint(cellId,x);
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

      pts[0] = cellId;
      verts->InsertNextCell(1,pts);
      }

    output->SetVerts(verts);
    verts->Delete();
    outCD->PassData(inCD); //only if verts are generated
    }

  // clean up and update output
  output->SetPoints(newPts);
  newPts->Delete();

  outPD->PassData(inCD); //because number of points = number of cells
  if (weights)
    {
    delete [] weights;
    }
}

void vtkCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}

