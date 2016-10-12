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

int TestPolygonBuilder(int, char* [])
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkIdType a = points->InsertNextPoint(0,0,0);
  vtkIdType b = points->InsertNextPoint(1,0,0);
  vtkIdType c = points->InsertNextPoint(0,1,0);
  vtkIdType d = points->InsertNextPoint(1,1,0);
  vtkIdType e = points->InsertNextPoint(0.5,0.5,0);

  // The ordering of the vertices ensures that the normals of all of the
  // subtriangles are in the same direction (0,0,1)
  vtkIdType triangles[4][3] = {{e,c,a},
                               {e,a,b},
                               {e,b,d},
                               {e,d,c}};

  vtkPolygonBuilder builder;

  vtkIdType p[3];
  for (size_t i=0;i<4;i++)
  {
    for (size_t j=0;j<3;j++)
      p[j] = triangles[i][j];
    builder.InsertTriangle(p);
  }

  vtkNew<vtkIdListCollection> polys;
  builder.GetPolygons(polys.GetPointer());

  if (polys->GetNumberOfItems()!=1)
  {
    return EXIT_FAILURE;
  }

  vtkIdList* poly = polys->GetItem(0);
  if(poly->GetNumberOfIds()!=4)
  {
    return EXIT_FAILURE;
  }
  poly->Delete();
  polys->RemoveAllItems();

  return EXIT_SUCCESS;
}
