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

int TestPolygonBuilder3(int, char* [])
{

  vtkPolygonBuilder builder;
  builder.InsertTriangle(NULL);
  vtkNew<vtkIdListCollection> polys;
  builder.GetPolygons(polys.GetPointer());
  if (polys->GetNumberOfItems() != 0)
  {
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}
