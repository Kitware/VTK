/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestProjectSphereFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkProjectSphereFilter.h"

#include "vtkCellIterator.h"
#include "vtkSphereSource.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPointLocator.h"
#include "vtkFloatArray.h"

#include "vtkVertex.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"

#include "vtkExecutive.h"
#include "vtkCommand.h"
#include "vtkTestErrorObserver.h"

#include <sstream>

static int ComparePolyData (vtkPolyData *p1, vtkPolyData *p2);
int UnitTestProjectSphereFilter(int, char*[])
{
  int status = EXIT_SUCCESS;

  {
    // Print
    std::cout << "  Testing print...";
    std::ostringstream emptyPrint;
    vtkSmartPointer<vtkProjectSphereFilter> filter =
      vtkSmartPointer<vtkProjectSphereFilter>::New();
    double center[3];
    center[0] = 1.0; center[1] = 2.0; center[2] = 3.0;
    filter->SetCenter(center);
    filter->KeepPolePointsOff();
    filter->TranslateZOn();
    filter->Print(emptyPrint);
    std::cout << "PASSED" << std::endl;
  }
  {
    std::cout << "  Testing errors...";
    vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
      vtkSmartPointer<vtkTest::ErrorObserver>::New();
    vtkSmartPointer<vtkTest::ErrorObserver>  executiveObserver =
      vtkSmartPointer<vtkTest::ErrorObserver>::New();

    vtkSmartPointer<vtkProjectSphereFilter> filter =
      vtkSmartPointer<vtkProjectSphereFilter>::New();
    filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, executiveObserver);
    vtkSmartPointer<vtkPolyData> badPoly =
      vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkVertex> aVertex =
      vtkSmartPointer<vtkVertex>::New();
    aVertex->GetPointIds()->SetId(0,0);
    aVertex->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
    vtkSmartPointer<vtkCellArray> vertices =
      vtkSmartPointer<vtkCellArray>::New();
    vertices->InsertNextCell(aVertex);
    badPoly->SetVerts(vertices);
    filter->SetInputData(badPoly);
    filter->Update();

    int status1 =  errorObserver->CheckErrorMessage("Can only deal with vtkPolyData polys");
    if (status1 == 0)
    {
      std::cout << "PASSED" << std::endl;
    }
    else
    {
      status++;
      std::cout << "FAILED" << std::endl;
    }
  }
  {
    int status1;
    std::cout << "Testing compare polydata...";
    vtkSmartPointer<vtkSphereSource> source =
      vtkSmartPointer<vtkSphereSource>::New();
    source->Update();

    vtkSmartPointer<vtkPolyData> polyData1 =
      vtkSmartPointer<vtkPolyData>::New();
    polyData1->DeepCopy(source->GetOutput());

    vtkSmartPointer<vtkProjectSphereFilter> filter =
      vtkSmartPointer<vtkProjectSphereFilter>::New();
    filter->SetInputConnection(source->GetOutputPort());
    filter->Update();

    vtkSmartPointer<vtkPolyData> polyData2 =
      vtkSmartPointer<vtkPolyData>::New();
    polyData2->DeepCopy(source->GetOutput());

    status1 = ComparePolyData(polyData1, polyData2);
    if (status1 != 0)
    {
      std::cout << "Failed" << std::endl;
    }
    else
    {
      std::cout << "Passed" << std::endl;
    }
    status += status1;
  }
  return status;
}

int ComparePolyData (vtkPolyData *p1, vtkPolyData *p2)
{
  int status = 0;
  if (p1->GetNumberOfCells() != p2->GetNumberOfCells())
  {
    std::cout << "ERROR: ComparePolyData - p1->GetNumberOfCells() "
              << p1->GetNumberOfCells() << " != "
              << "p2->GetNumberOfCells() "
              << p2->GetNumberOfCells() << std::endl;
    status++;
  }
  vtkIdList *pointIdList1;
  vtkIdType *ptIds1;
  int numCellPts1;
  vtkIdList *pointIdList2;
  vtkIdType *ptIds2;
  int numCellPts2;

  vtkSmartPointer<vtkCellIterator> cellIter1 =
      vtkSmartPointer<vtkCellIterator>::Take(p1->NewCellIterator());
  vtkSmartPointer<vtkCellIterator> cellIter2 =
      vtkSmartPointer<vtkCellIterator>::Take(p2->NewCellIterator());
  for (cellIter1->InitTraversal(), cellIter2->InitTraversal();
       cellIter1->IsDoneWithTraversal();
       cellIter1->GoToNextCell(),cellIter2->GoToNextCell())
  {
    pointIdList1 = cellIter1->GetPointIds();
    numCellPts1 = pointIdList1->GetNumberOfIds();
    ptIds1 = pointIdList1->GetPointer(0);
    pointIdList2 = cellIter2->GetPointIds();
    numCellPts2 = pointIdList2->GetNumberOfIds();
    ptIds2 = pointIdList2->GetPointer(0);

    if (numCellPts1 != numCellPts2)
      {
      std::cout << "numCellPts1 != numCellPts2 " << numCellPts1 << " != " << numCellPts2 << std::endl;
      return 1;
      }
    for (vtkIdType i = 0; i < numCellPts1; ++i)
    {
      if (ptIds1[i] != ptIds2[i])
      {
        std::cout << ptIds1[i] << " != " << ptIds2[i] << std::endl;
      }
    }
  }
  vtkPointData *pointData1;
  pointData1 = p1->GetPointData();
  vtkPointData *pointData2;
  pointData2 = p2->GetPointData();

  vtkSmartPointer<vtkFloatArray> normals1 =
    vtkFloatArray::SafeDownCast(pointData1->GetNormals());
  vtkSmartPointer<vtkFloatArray> normals2 =
    vtkFloatArray::SafeDownCast(pointData2->GetNormals());

  for (vtkIdType i = 0; i < normals1->GetNumberOfTuples(); ++i)
    {
    double normal1[3], normal2[3];
    normals1->GetTuple(i, normal1);
    normals2->GetTuple(i, normal2);
    for (int j = 0; j < 3; ++j)
      {
      if (normal1[j] != normal2[j])
        {
          std::cout << "Cell: " << i << " normal1[" << j << "] != normal2 "
                    << j << "] " << normal1[j] << " != " << normal2[j]
                    << std::endl;
          ++status;
        }
      }
    }
  return status;
}
