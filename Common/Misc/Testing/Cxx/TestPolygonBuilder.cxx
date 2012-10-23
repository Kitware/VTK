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
#include "vtkSmartPointer.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygonBuilder.h"
#include "vtkIdList.h"

int TestPolygonBuilder(int, char* [])
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkIdType a = points->InsertNextPoint(0,0,0);
  vtkIdType b = points->InsertNextPoint(1,0,0);
  vtkIdType c = points->InsertNextPoint(0,1,0);
  vtkIdType d = points->InsertNextPoint(1,1,0);

  vtkPolygonBuilder builder;

  vtkIdType abc[3]={a,b,c};
  if(builder.InsertTriangle(abc)==false)
    {
    return EXIT_FAILURE;
    }

  vtkIdType bcd[3]={b,c,d}; //invalid triangle
  if(builder.InsertTriangle(bcd)==true)  //check whether we correctly fail
    {
    return EXIT_FAILURE;
    }

  vtkIdType cbd[3]={c,b,d};
  if(builder.InsertTriangle(cbd)==false)
    {
    return EXIT_FAILURE;
    }

  vtkNew<vtkIdList> poly;
  builder.GetPolygon(poly.GetPointer());

  if(poly->GetNumberOfIds()!=4)
    {
    return EXIT_FAILURE;
    }

  vtkIdType expected[4]= {0,1,3,2};
  for(size_t i =0 ;i<4; i++)
    {
    if(poly->GetId(i)!=expected[i])
      {
      return EXIT_FAILURE;
      }
    }
  return EXIT_SUCCESS;
}
