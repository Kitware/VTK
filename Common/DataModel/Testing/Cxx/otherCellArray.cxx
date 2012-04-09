/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherCellArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the CellArray

#include "vtkDebugLeaks.h"

#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkQuad.h"

#include <vtksys/ios/sstream>

int TestCellArray(ostream& strm)
{
  // actual test
  strm << "Test CellArray Start" << endl;
  vtkCellArray *ca = vtkCellArray::New();

  ca->Initialize();
  strm << "ca->GetNumberOfCells() = " << ca->GetNumberOfCells() << endl;
  strm << "ca->GetSize() = " << ca->GetSize() << endl;
  strm << "ca->GetNumberOfConnectivityEntries() = " << ca->GetNumberOfConnectivityEntries() << endl;
  strm << "ca->EstimateSize (1000, 3) = " << ca->EstimateSize(1000,3) << endl;

  vtkIdType npts = 3;
  vtkIdType pts[3] = {0, 1, 2};
  vtkQuad *cell = vtkQuad::New();
  vtkIdList *ids = vtkIdList::New();
  vtkIdType *ptrIds = ids->WritePointer(0,3);
  memcpy (ptrIds, pts, 3 * sizeof (vtkIdType));

  strm << "ca->InsertNextCell (npts, pts) = " << ca->InsertNextCell (npts, pts) << endl;
  strm << "ca->InsertNextCell (cell) = " << ca->InsertNextCell (cell) << endl;
  strm << "ca->InsertNextCell (ids) = " << ca->InsertNextCell (ids) << endl;
  strm << "ca->InsertNextCell (4) = " << ca->InsertNextCell (4) << endl;
  ca->InsertCellPoint (3);
  ca->InsertCellPoint (4);
  ca->InsertCellPoint (5);

  ca->InsertCellPoint (6);
  ca->InsertCellPoint (7);
  ca->InsertCellPoint (8);
  ca->UpdateCellCount (3);

  strm << "ca->GetNumberOfCells() = " << ca->GetNumberOfCells() << endl;
  strm << "ca->GetSize() = " << ca->GetSize() << endl;
  strm << "ca->GetNumberOfConnectivityEntries() = " << ca->GetNumberOfConnectivityEntries() << endl;

  vtkIdTypeArray *cells = vtkIdTypeArray::New();
  cells->SetNumberOfTuples(12);
  vtkIdType idT[12] = {3, 0, 1, 2, 3, 1, 2, 3, 3, 3, 4, 5};
  cells->SetVoidArray(idT,12,1);
  ca->Allocate(10000);
  ca->SetCells (3, cells);
  strm << "ca->GetNumberOfCells() = " << ca->GetNumberOfCells() << endl;
  strm << "ca->GetSize() = " << ca->GetSize() << endl;
  strm << "ca->GetNumberOfConnectivityEntries() = " << ca->GetNumberOfConnectivityEntries() << endl;

  ca->Delete();
  cell->Delete();
  ids->Delete();
  cells->Delete();
  strm << "Test CellArray Complete" << endl;

  return 0;
}

int otherCellArray(int,char *[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;
  return TestCellArray(vtkmsg_with_warning_C4701);
}
