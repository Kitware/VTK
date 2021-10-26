/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMappedGridDeepCopy

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
  This test was written by Menno Deij - van Rijswijk (MARIN).
----------------------------------------------------------------------------*/

#include "vtkCell.h" // for cell types
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkDebugLeaks.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <algorithm>
#include <fstream>

int TestMappedGridDeepCopy(int vtkNotUsed(argc), char*[] vtkNotUsed(argv))
{
  vtkUnstructuredGrid* original;
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&original);

  vtkUnstructuredGridBase* mg;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&mg);

  // copy the mapped grid into a normal unstructured grid.
  // copying will proceed via the super class
  // vtkUnstructuredGridBase::DeepCopy function
  // implementation that uses a cell iterator. This will
  // invoke to InsertNextCell function with face list
  // for *all* cells (even if they are not VTK_POLYHEDRON).
  // In the old implementation this gave copy errors. The fix
  // proposed together with this test addresses that issue.
  vtkNew<vtkUnstructuredGrid> copy;
  copy->Allocate(mg->GetNumberOfCells());
  copy->DeepCopy(mg);

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
  mg->Delete();
  return EXIT_SUCCESS;
}
