/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkReflectionFilter

#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellType.h>
#include <vtkIdList.h>
#include <vtkReflectionFilter.h>
#include <iostream>
#include <assert.h>

#define AssertMacro(b) if(!(b)){std::cerr <<"Failed to reflect pyramid"<<std::endl;return EXIT_FAILURE;}

int TestReflectionFilter(int, char *[])
{

  vtkSmartPointer<vtkUnstructuredGrid> pyramid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    {
    vtkSmartPointer<vtkPoints> points =  vtkSmartPointer<vtkPoints>::New();
    points->InsertNextPoint(-1,-1,-1);
    points->InsertNextPoint( 1,-1,-1);
    points->InsertNextPoint( 1, 1,-1);
    points->InsertNextPoint(-1, 1,-1);
    points->InsertNextPoint(0,0,1);
    pyramid->SetPoints(points);
    }

  vtkNew<vtkIdList> verts;
  verts->InsertNextId(0);
  verts->InsertNextId(1);
  verts->InsertNextId(2);
  verts->InsertNextId(3);
  verts->InsertNextId(4);

  pyramid->InsertNextCell(VTK_PYRAMID,verts.GetPointer());


  for(int i=0; i<2; i++)
    {
    vtkSmartPointer<vtkReflectionFilter> reflectionFilter =  vtkSmartPointer<vtkReflectionFilter>::New();
    reflectionFilter->SetInputData(pyramid.GetPointer());
    i==0? reflectionFilter->CopyInputOff() : reflectionFilter->CopyInputOn();
    reflectionFilter->SetPlaneToZMin();
    reflectionFilter->Update();
    vtkUnstructuredGrid* pyramid1 = vtkUnstructuredGrid::SafeDownCast(reflectionFilter->GetOutput());
    vtkNew<vtkIdList> cellIds;
    if(i==0)
      {
      AssertMacro(pyramid1->GetNumberOfCells()==1);
      }
    else
      {
      AssertMacro(pyramid1->GetNumberOfCells()==2);
      }
    pyramid1->GetCellPoints(i, cellIds.GetPointer());
    int apex = cellIds->GetId(4);
    int offset = i==0? 0 : 5;
    AssertMacro(apex==4+offset);
    for(int j=0; j<4; j++)
      {
      int next = cellIds->GetId((j+1)%4);
      int nextExpected = (cellIds->GetId(j)-offset+3)%4+offset;
      AssertMacro(next ==nextExpected);
      }
    }


  return EXIT_SUCCESS;
}
