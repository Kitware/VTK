/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTecPlotReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkTecplotReader
// .SECTION Description
//

#include "vtkOBJReader.h"
#include "vtkDebugLeaks.h"

#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"


int TestOBJReaderRelative( int argc, char *argv[] )
{
  int retVal = 0;

  // Create the reader.
  char* fname_rel = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/relative_indices.obj");
  vtkSmartPointer<vtkOBJReader> reader_rel =
    vtkSmartPointer<vtkOBJReader>::New();
  reader_rel->SetFileName(fname_rel);
  reader_rel->Update();
  delete [] fname_rel;

  // Create the reader.
  char* fname_abs = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/absolute_indices.obj");
  vtkSmartPointer<vtkOBJReader> reader_abs =
    vtkSmartPointer<vtkOBJReader>::New();
  reader_abs->SetFileName(fname_abs);
  reader_abs->Update();
  delete [] fname_abs;

  vtkPolyData *data_rel = reader_rel->GetOutput();
  vtkPolyData *data_abs = reader_abs->GetOutput();

#define CHECK(obj, method)                                            \
  if (obj##_rel->method != obj##_abs->method)                         \
  {                                                                 \
    cerr << "Error: different values for " #obj "->" #method << endl; \
    retVal = 1;                                                       \
  }
#define CHECK_ARRAY(obj, idx)                                              \
  if (obj##_rel[idx] != obj##_abs[idx])                                    \
  {                                                                      \
    cerr << "Error: different values for " #obj "[" << (idx) << "]" << endl; \
    retVal = 1;                                                            \
  }
#define CHECK_SCALAR(obj)                                \
  if (obj##_rel != obj##_abs)                            \
  {                                                    \
    cerr << "Error: different values for " #obj << endl; \
    retVal = 1;                                          \
  }

  CHECK(data, GetNumberOfVerts())
  CHECK(data, GetNumberOfLines())
  CHECK(data, GetNumberOfCells())
  CHECK(data, GetNumberOfStrips())

  vtkCellArray *polys_rel = data_rel->GetPolys();
  vtkCellArray *polys_abs = data_abs->GetPolys();

  CHECK(polys, GetNumberOfCells());

  vtkIdType npts_rel;
  vtkIdType npts_abs;
  vtkIdType *pts_rel;
  vtkIdType *pts_abs;

  polys_rel->InitTraversal();
  polys_abs->InitTraversal();
  while (!polys_rel->GetNextCell(npts_rel, pts_rel) &&
         !polys_abs->GetNextCell(npts_abs, pts_abs))
  {
    CHECK_SCALAR(npts)

    for (vtkIdType i = 0; i < npts_rel && i < npts_abs; ++i)
    {
      CHECK_ARRAY(pts, i)
    }
  }

#undef CHECK_SCALAR
#undef CHECK_ARRAY
#undef CHECK

  return retVal;
}
