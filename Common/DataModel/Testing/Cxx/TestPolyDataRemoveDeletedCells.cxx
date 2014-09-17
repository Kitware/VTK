/*=========================================================================

  Program:   Visualization Toolkit
  Module:  TestPolyDataRemoveDeletedCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkIdList.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIntArray.h"

int TestPolyDataRemoveDeletedCells (int, char*[])
{
  int numCells = 8;

  vtkPoints *pts = vtkPoints::New();

  pts->InsertNextPoint(1, 0, 0); // 0
  pts->InsertNextPoint(3, 0, 0); // 1
  pts->InsertNextPoint(5, 0, 0); // 2
  pts->InsertNextPoint(7, 0, 0); // 3

  pts->InsertNextPoint(0, 2, 0); // 4
  pts->InsertNextPoint(2, 2, 0); // 5
  pts->InsertNextPoint(4, 2, 0); // 6
  pts->InsertNextPoint(6, 2, 0); // 7

  pts->InsertNextPoint(9, 0, 0); // 8
  pts->InsertNextPoint(11, 0, 0); // 9
  pts->InsertNextPoint(8, 2, 0); // 10
  pts->InsertNextPoint(10, 2, 0); // 11

  vtkPolyData *pd = vtkPolyData::New();
  pd->Allocate(numCells);
  pd->SetPoints(pts);

  pts->Delete();

  vtkIdList *cell;
  cell = vtkIdList::New();

  // adding different cells in arbitrary order

  // 1

  cell->InsertNextId(4);
  cell->InsertNextId(0);
  cell->InsertNextId(5);
  cell->InsertNextId(1);
  cell->InsertNextId(6);

  pd->InsertNextCell(VTK_TRIANGLE_STRIP, cell);

  cell->Reset();

  // 2

  cell->InsertNextId(1);
  cell->InsertNextId(6);
  cell->InsertNextId(2);
  cell->InsertNextId(7);
  cell->InsertNextId(3);

  pd->InsertNextCell(VTK_TRIANGLE_STRIP, cell);

  cell->Reset();

  // 3

  cell->InsertNextId(0);

  pd->InsertNextCell(VTK_VERTEX, cell);

  cell->Reset();

  // 4

  cell->InsertNextId(0);
  cell->InsertNextId(1);
  cell->InsertNextId(2);
  cell->InsertNextId(3);
  cell->InsertNextId(8);
  cell->InsertNextId(11);
  cell->InsertNextId(10);
  cell->InsertNextId(7);
  cell->InsertNextId(6);
  cell->InsertNextId(5);
  cell->InsertNextId(4);

  pd->InsertNextCell(VTK_POLY_LINE, cell);

  cell->Reset();

  // 5

  cell->InsertNextId(3);
  cell->InsertNextId(8);
  cell->InsertNextId(9);
  cell->InsertNextId(11);
  cell->InsertNextId(10);
  cell->InsertNextId(7);

  pd->InsertNextCell(VTK_POLYGON, cell);

  cell->Reset();

  // 6

  cell->InsertNextId(1);

  pd->InsertNextCell(VTK_VERTEX, cell);

  cell->Reset();

  // 7

  cell->InsertNextId(3);
  cell->InsertNextId(10);

  pd->InsertNextCell(VTK_LINE, cell);

  cell->Reset();

  // 8

  cell->InsertNextId(8);
  cell->InsertNextId(9);
  cell->InsertNextId(11);

  pd->InsertNextCell(VTK_TRIANGLE, cell);

  cell->Delete();

  vtkIntArray *scalars = vtkIntArray::New();

  scalars->InsertNextValue(VTK_TRIANGLE_STRIP);
  scalars->InsertNextValue(VTK_TRIANGLE_STRIP);
  scalars->InsertNextValue(VTK_VERTEX);
  scalars->InsertNextValue(VTK_POLY_LINE);
  scalars->InsertNextValue(VTK_POLYGON);
  scalars->InsertNextValue(VTK_VERTEX);
  scalars->InsertNextValue(VTK_LINE);
  scalars->InsertNextValue(VTK_TRIANGLE);

  pd->GetCellData()->SetScalars(scalars);

  scalars->Delete();

  // delete some cells

  pd->DeleteCell(0);
  pd->DeleteCell(3);
  pd->DeleteCell(5);
  pd->DeleteCell(7);

  int types[] = {
    //VTK_TRIANGLE_STRIP,
    VTK_TRIANGLE_STRIP,
    VTK_VERTEX,
    //VTK_POLY_LINE,
    VTK_POLYGON,
    //VTK_VERTEX,
    VTK_LINE,
    //VTK_TRIANGLE
  };

  pd->RemoveDeletedCells();

  vtkIntArray *newScalars = vtkIntArray::SafeDownCast(pd->GetCellData()->GetScalars());

  int retVal = 0;

  if (pd->GetNumberOfCells() != numCells-4)
    {
    std::cout << "Number of cells does not match!" << std::endl;
    retVal = 1;
    }

  for (vtkIdType i = 0; i < pd->GetNumberOfCells(); i++)
    {
    if (pd->GetCellType(i) != newScalars->GetValue(i))
      {
      std::cout << "Cell " << i << " has wrong data. Value should be " << pd->GetCellType(i)
                << " but is " << newScalars->GetValue(i) << std::endl;
      retVal = 1;
      }

    if (pd->GetCellType(i) != types[i])
      {
      std::cout << "Cell " << i << " has wrong type. Value should be " << pd->GetCellType(i)
                << " but is " << newScalars->GetValue(i) << std::endl;
      retVal = 1;
      }
    }

  pd->Delete();

  return retVal;
}
