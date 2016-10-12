/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygonBuilder4.cxx

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

int TestPolygonBuilder4(int, char* [])
{

  vtkPolygonBuilder builder;
  vtkNew<vtkIdListCollection> polys;

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkIdType a = points->InsertNextPoint(0,0,0);
  vtkIdType b = points->InsertNextPoint(1,0,0);
  vtkIdType c = points->InsertNextPoint(0,1,0);
  vtkIdType d = points->InsertNextPoint(1,1,0);

  // two counter-rotated triangles give unexpected results
#define NTRIANGLES 4
  vtkIdType triangles[NTRIANGLES][3] = {
        {b,c,a},{d,c,b},{c,b,a},{d,b,c}
        };

  vtkIdType p[3];
  for (size_t i=0;i<NTRIANGLES;i++)
  {
    for (size_t j=0;j<3;j++)
    {
      p[j] = triangles[i][j];
    }
    builder.InsertTriangle(p);
  }

  builder.GetPolygons(polys.GetPointer());

  vtkIdType expected(1);
  if (polys->GetNumberOfItems()!=1) // and a-b-c-d expected
  {
    vtkGenericWarningMacro(<< "number of items is " << polys->GetNumberOfItems() << " but expected " << expected << endl);
    return EXIT_FAILURE;
  }

  vtkIdList* poly = polys->GetItem(0);
  expected = 4;
  if(poly->GetNumberOfIds()!=4)
  {
    vtkGenericWarningMacro(<< "number of ids is " << poly->GetNumberOfIds() << " but expected " << expected << endl);
    return EXIT_FAILURE;
  }
  poly->Delete();
  polys->RemoveAllItems();

  return EXIT_SUCCESS;
}
