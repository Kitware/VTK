/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdListCollection.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygonBuilder.h"
#include "vtkSmartPointer.h"

int TestPolygonBuilder5(int, char*[])
{
  // this case comes from a real-world example (see https://gitlab.kitware.com/vtk/vtk/issues/17170
  // )
#define NTRIANGLES 7

  vtkIdType tris[NTRIANGLES][3] = { { 0, 1, 2 }, { 3, 4, 7 }, { 7, 4, 5 }, { 4, 6, 5 }, { 3, 7, 4 },
    { 4, 7, 6 }, { 7, 5, 6 } };

  vtkPolygonBuilder builder;
  vtkIdType p[3];
  for (int i = 0; i < NTRIANGLES; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      p[j] = tris[i][j];
    }

    builder.InsertTriangle(p);
  }

  vtkNew<vtkIdListCollection> polys;
  builder.GetPolygons(polys.GetPointer()); // will result in out-of-memory crash

  if (polys->GetNumberOfItems() < 1)
  {
    return EXIT_FAILURE;
  }

  // clean up after ourselves.
  for (int i = 0; i < polys->GetNumberOfItems(); ++i)
  {
    polys->GetItem(i)->Delete();
  }

  polys->RemoveAllItems();

  return EXIT_SUCCESS;
}
