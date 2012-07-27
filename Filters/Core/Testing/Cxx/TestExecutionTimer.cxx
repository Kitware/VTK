/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkExecutionTimer.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>

//
// This test uses the guts of TestAppendPolyData.  I just attach a
// vtkExecutionTimer to vtkAppendPolyData so that I can watch
// something non-trivial.

int TestExecutionTimer(int, char *[])
{
  vtkSmartPointer<vtkPoints> points1 = vtkSmartPointer<vtkPoints>::New();
  points1->InsertNextPoint(0,0,0);
  points1->InsertNextPoint(1,1,1);

  vtkSmartPointer<vtkPoints> points2 = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType pid[1];
  pid[0] = points2->InsertNextPoint(5,5,5);
  vertices->InsertNextCell ( 1,pid );
  pid[0] = points2->InsertNextPoint(6,6,6);
  vertices->InsertNextCell ( 1,pid );

  vtkSmartPointer<vtkPolyData> polydata1 = vtkSmartPointer<vtkPolyData>::New();
  polydata1->SetPoints(points1);
  polydata1->SetVerts(vertices);

  vtkSmartPointer<vtkPolyData> polydata2 = vtkSmartPointer<vtkPolyData>::New();
  polydata2->SetPoints(points2);

  vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
  appendFilter->AddInputData(polydata1);
  appendFilter->AddInputData(polydata2);

  vtkSmartPointer<vtkExecutionTimer> timer = vtkSmartPointer<vtkExecutionTimer>::New();
  timer->SetFilter(appendFilter);
  appendFilter->Update();

  std::cout << "TestExecutionTimer: Filter under inspection (vtkAppendPolyData)\n"
            << "took " << timer->GetElapsedCPUTime() << " seconds of CPU time \n"
            << "and " << timer->GetElapsedWallClockTime() << " seconds of wall "
            << "clock time to execute.\n";

  // As long as the thing executes without crashing, the test is
  // successful.
  return EXIT_SUCCESS;
}
