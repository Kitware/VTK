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

//-----------------------------------------------------------------------------
int CheckArrayPointData(vtkDataArray* firstArray, vtkDataArray* secondArray, int idx)
{
  // Check that each component at a given index are the same in each array
  for (int compIdx = 0; compIdx < secondArray->GetNumberOfComponents(); ++compIdx)
  {
    if (firstArray->GetComponent(idx, compIdx) != secondArray->GetComponent(idx, compIdx))
    {
      cerr << "Error: different values for " "[" << (idx) << "]_"<< compIdx << endl;
      return 1;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
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

#define CHECK_ARRAY_EXISTS(array)          \
  if (!array)                              \
  {                                        \
  cerr << "Array does not exist." << endl; \
  retVal = 1;                              \
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

  // Get the texture and normal arrays to check
  vtkDataArray* tcoords_rel = data_rel->GetPointData()->GetTCoords();
  vtkDataArray* tcoords_abs = data_abs->GetPointData()->GetTCoords();

  CHECK_ARRAY_EXISTS(tcoords_rel)
  CHECK_ARRAY_EXISTS(tcoords_abs)

  int tcoordsNbComp_rel = tcoords_rel->GetNumberOfComponents();
  int tcoordsNbComp_abs = tcoords_abs->GetNumberOfComponents();

  vtkDataArray* normals_rel = data_rel->GetPointData()->GetNormals();
  vtkDataArray* normals_abs = data_abs->GetPointData()->GetNormals();

  CHECK_ARRAY_EXISTS(normals_rel)
  CHECK_ARRAY_EXISTS(normals_abs)

  int normalsNbComp_rel = normals_rel->GetNumberOfComponents();
  int normalsNbComp_abs = normals_abs->GetNumberOfComponents();

  CHECK_SCALAR(tcoordsNbComp)
  CHECK_SCALAR(normalsNbComp)

  while (polys_rel->GetNextCell(npts_rel, pts_rel) &&
         polys_abs->GetNextCell(npts_abs, pts_abs))
  {
    CHECK_SCALAR(npts)

    for (vtkIdType i = 0; i < npts_rel && i < npts_abs; ++i)
    {
      CHECK_ARRAY(pts, i)

      // For each points, check if the point data associated with the points
      // from the OBJ using relative coordinates matches the ones from the
      // OBJ using absolute coordinates
      retVal = CheckArrayPointData(tcoords_rel, tcoords_abs, i)
      || CheckArrayPointData(normals_rel, normals_abs, i);
    }
  }

#undef CHECK_ARRAY_EXISTS
#undef CHECK_SCALAR
#undef CHECK_ARRAY
#undef CHECK

  return retVal;
}
