// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIdListCollection.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygonBuilder.h"
#include "vtkSmartPointer.h"

int TestPolygonBuilder3(int, char*[])
{

  vtkPolygonBuilder builder;
  builder.InsertTriangle(nullptr);
  vtkNew<vtkIdListCollection> polys;
  builder.GetPolygons(polys);
  if (polys->GetNumberOfItems() != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
