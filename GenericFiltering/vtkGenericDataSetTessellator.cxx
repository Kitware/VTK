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
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"

vtkCxxRevisionMacro(vtkGenericDataSetTessellator, "1.1");
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
  vtkGenericDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  if (input == NULL)
    {
    return;
    }

  vtkDebugMacro(<< "vtkGenericDataSetTessellator");

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkGenericAdaptorCell *cell;

  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  
  // Copy original points and point data
  vtkPoints *newPts = vtkPoints::New();
  
  output->Allocate(numCells);
  output->SetPoints(newPts);

  // Estimate output size:
  vtkIdType estimatedSize = input->GetEstimatedSize();
  newPts->Allocate(estimatedSize, numPts);

  // loop over region
  vtkCellArray *array = vtkCellArray::New();
  array->Allocate(numCells);

  vtkGenericCellIterator *cellIt = input->NewCellIterator();
  for(cellIt->Begin(); !cellIt->IsAtEnd(); cellIt->Next())
    {
    cell = cellIt->GetCell();
    cell->Tessellate(input->GetAttributes(), input->GetTessellator(),newPts,
                     array,outputPD, outputCD);
    } //for all cells
  cellIt->Delete();
  
  if( input->GetCellDimension() == 3)
    {
    output->SetCells(VTK_TETRA, array);
    }
  else if( input->GetCellDimension() == 2)
    {
    output->SetCells(VTK_TRIANGLE, array);
    }
  else
    {
    vtkErrorMacro(<<"Cell dimension not supported " << input->GetCellDimension() );
    }
  vtkDebugMacro(<<"Subdivided " << numCells << " cells");

  newPts->Delete();
  array->Delete();

  output->Squeeze();  
}

//----------------------------------------------------------------------------
void vtkGenericDataSetTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
