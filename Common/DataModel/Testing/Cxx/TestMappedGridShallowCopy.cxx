// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*----------------------------------------------------------------------------
  This was taken from TestMappedGridDeepCopy and modified by Bengt Rosenberger.
----------------------------------------------------------------------------*/

#include "vtkCell.h" // for cell types
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkDebugLeaks.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <algorithm>
#include <fstream>

int TestMappedGridShallowCopy(int vtkNotUsed(argc), char*[] vtkNotUsed(argv))
{
  vtkUnstructuredGridBase* original;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&original);

  // This simulates the executive calling vtkUnstructuredGrid::Initialize
  // when preparing an algorithm output
  vtkUnstructuredGrid* copy = vtkUnstructuredGrid::New();
  copy->Initialize();

  // Make sure shallow copy succeeds after call to Initialize
  copy->ShallowCopy(original);

  // Compare number of points
  if (copy->GetNumberOfPoints() != original->GetNumberOfPoints())
  {
    cerr << "Number of points do not match" << endl;
    return EXIT_FAILURE;
  }

  // Compare number of cells
  if (copy->GetNumberOfCells() != original->GetNumberOfCells())
  {
    cerr << "Number of cells do not match" << endl;
    return EXIT_FAILURE;
  }

  // Comparison code below taken from TestMappedGridDeepCopy.cxx
  vtkCellIterator* oIt = original->NewCellIterator();
  vtkCellIterator* cIt = copy->NewCellIterator();

  vtkNew<vtkGenericCell> orig, copied;
  for (cIt->InitTraversal(), oIt->InitTraversal();
       !cIt->IsDoneWithTraversal() && !oIt->IsDoneWithTraversal();
       cIt->GoToNextCell(), oIt->GoToNextCell())
  {
    oIt->GetCell(orig.GetPointer());
    cIt->GetCell(copied.GetPointer());

    if (cIt->GetCellType() != oIt->GetCellType())
    {
      cerr << "Cell types do not match" << endl;
      return EXIT_FAILURE;
    }

    if (cIt->GetCellType() == VTK_POLYHEDRON)
    {
      vtkIdList* oFaces = oIt->GetFaces();
      vtkIdList* cFaces = cIt->GetFaces();

      if (cFaces->GetNumberOfIds() != oFaces->GetNumberOfIds())
      {
        cerr << "Face id list length does not match" << endl;
        cerr << "Original: ";
        for (vtkIdType i = 0; i < oFaces->GetNumberOfIds(); ++i)
        {
          cerr << oFaces->GetId(i) << " ";
        }
        cerr << endl;

        cerr << "Copied:   ";
        for (vtkIdType i = 0; i < cFaces->GetNumberOfIds(); ++i)
          cerr << cFaces->GetId(i) << " ";
        cerr << endl;

        return EXIT_FAILURE;
      }

      for (vtkIdType i = 0; i < cFaces->GetNumberOfIds(); ++i)
      {
        vtkIdType c = cFaces->GetId(i);
        vtkIdType o = oFaces->GetId(i);

        if (c != o)
        {
          cerr << "Face id list content does not match at" << i << endl;
          return EXIT_FAILURE;
        }
      }
    }
  }
  oIt->Delete();
  cIt->Delete();

  original->Delete();
  copy->Delete();
  return EXIT_SUCCESS;
}
