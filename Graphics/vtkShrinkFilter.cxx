/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkFilter.cxx
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
#include "vtkShrinkFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkShrinkFilter, "1.57");
vtkStandardNewMacro(vtkShrinkFilter);

vtkShrinkFilter::vtkShrinkFilter(float sf)
{
  sf = ( sf < 0.0 ? 0.0 : (sf > 1.0 ? 1.0 : sf));
  this->ShrinkFactor = sf;
}

void vtkShrinkFilter::Execute()
{
  vtkPoints *newPts;
  int i, j, numIds, abort=0;
  vtkIdType cellId, numCells, numPts;
  vtkIdType oldId, newId;
  float center[3], *p, pt[3];
  vtkPointData *pd, *outPD;;
  vtkIdList *ptIds, *newPtIds;
  vtkDataSet *input= this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkIdType tenth;
  float decimal;

  vtkDebugMacro(<<"Shrinking cells");

  numCells=input->GetNumberOfCells();
  numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
    {
    vtkErrorMacro(<<"No data to shrink!");
    return;
    }

  ptIds = vtkIdList::New();
  ptIds->Allocate(VTK_CELL_SIZE);
  newPtIds = vtkIdList::New();
  newPtIds->Allocate(VTK_CELL_SIZE);

  output->Allocate(numCells);
  newPts = vtkPoints::New();
  newPts->Allocate(numPts*8,numPts);
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd,numPts*8,numPts);

  // Traverse all cells, obtaining node coordinates.  Compute "center" of cell,
  // then create new vertices shrunk towards center.
  //
  tenth   = numCells/10 + 1;
  decimal = 0.0;

  for (cellId=0; cellId < numCells && !abort; cellId++)
    {
    input->GetCellPoints(cellId, ptIds);
    numIds = ptIds->GetNumberOfIds();

    //abort/progress methods
    if (cellId % tenth == 0) 
      {
      decimal += 0.1;
      this->UpdateProgress (decimal);
      abort = this->GetAbortExecute();
      }

    // get the center of the cell
    center[0] = center[1] = center[2] = 0.0;
    for (i=0; i < numIds; i++)
      {
      p = input->GetPoint(ptIds->GetId(i));
      for (j=0; j < 3; j++)
        {
        center[j] += p[j];
        }
      }
    for (j=0; j<3; j++)
      {
      center[j] /= numIds;
      }

    // Create new points and cells
    newPtIds->Reset();
    for (i=0; i < numIds; i++)
      {
      p = input->GetPoint(ptIds->GetId(i));
      for (j=0; j < 3; j++)
        {
        pt[j] = center[j] + this->ShrinkFactor*(p[j] - center[j]);
        }

      oldId = ptIds->GetId(i);
      newId = newPts->InsertNextPoint(pt);
      newPtIds->InsertId(i,newId);

      outPD->CopyData(pd, oldId, newId);
      }
    output->InsertNextCell(input->GetCellType(cellId), newPtIds);
    }//for all cells

  // Update ourselves and release memory
  //
  output->GetCellData()->PassData(input->GetCellData());

  output->SetPoints(newPts);
  output->Squeeze();

  ptIds->Delete();
  newPtIds->Delete();
  newPts->Delete();
}

void vtkShrinkFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
