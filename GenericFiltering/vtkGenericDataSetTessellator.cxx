/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSetTessellator.h"

#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkTetra.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"

vtkCxxRevisionMacro(vtkGenericDataSetTessellator, "1.2");
vtkStandardNewMacro(vtkGenericDataSetTessellator);

//----------------------------------------------------------------------------
//
vtkGenericDataSetTessellator::vtkGenericDataSetTessellator()
{
}

//----------------------------------------------------------------------------
vtkGenericDataSetTessellator::~vtkGenericDataSetTessellator()
{
}

//----------------------------------------------------------------------------
//
void vtkGenericDataSetTessellator::Execute()
{
  vtkDebugMacro(<< "Executing vtkGenericDataSetTessellator...");

  vtkGenericDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkGenericAdaptorCell *cell;
  vtkIdType numInserted=0, numNew, i;
  vtkIdType npts, *pts;

  // Copy original points and point data
  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(2*numPts,numPts);

  // loop over region
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::New();
  types->Allocate(numCells);
  vtkIdTypeArray *locs = vtkIdTypeArray::New();
  locs->Allocate(numCells);
  vtkCellArray *conn = vtkCellArray::New();
  conn->Allocate(numCells);

  vtkGenericCellIterator *cellIt = input->NewCellIterator();
  for(cellIt->Begin(); !cellIt->IsAtEnd(); cellIt->Next())
    {
    cell = cellIt->GetCell();
    cell->Tessellate(input->GetAttributes(), input->GetTessellator(),
                     newPts, conn, outputPD, outputCD);

    numNew = conn->GetNumberOfCells() - numInserted;
    numInserted = conn->GetNumberOfCells();
    
    for (i=0; i < numNew; i++) 
      {
      locs->InsertNextValue(conn->GetTraversalLocation());
      conn->GetNextCell(npts,pts); //side effect updates traversal location
      switch (cell->GetDimension())
        {
        case 1:
          types->InsertNextValue(VTK_LINE);
          break;
        case 2:
          types->InsertNextValue(VTK_TRIANGLE);
          break;
        case 3:
          types->InsertNextValue(VTK_TETRA);
          break;
        default:
          vtkErrorMacro(<<"Bad mojo in data set tessellation");
        } //switch
      } //insert each new cell
    } //for all cells
  cellIt->Delete();
  
  // Send to the output
  output->SetPoints(newPts);
  output->SetCells(types, locs, conn);

  vtkDebugMacro(<<"Subdivided " << numCells << " cells to produce "
                << conn->GetNumberOfCells() << "new cells");

  newPts->Delete();
  types->Delete();
  locs->Delete();
  conn->Delete();

  output->Squeeze();  
}

//----------------------------------------------------------------------------
void vtkGenericDataSetTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
