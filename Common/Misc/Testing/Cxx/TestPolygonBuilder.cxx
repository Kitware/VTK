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

  vtkIdType corner[4] = {a,b,c,d};

  vtkPolygonBuilder builder;

  vtkIdType p[3];
  p[0] = e;

  for (size_t i=0;i<4;i++)
    {
    p[1] = corner[i];
    p[2] = corner[(i+1)%4];
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
