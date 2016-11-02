/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkButterflySubdivisionFilter.h"
#include "vtkLinearSubdivisionFilter.h"
#include "vtkLoopSubdivisionFilter.h"

#include "vtkPolyData.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"


#include "vtkExecutive.h"
#include "vtkCommand.h"
#include "vtkTestErrorObserver.h"

#include <sstream>

#define CHECK_ERROR_MSG(errorObserver, msg, status)      \
  { \
  std::string expectedMsg(msg); \
  if (!errorObserver->GetError()) \
  { \
    std::cout << "Failed to catch any error. Expected the error message to contain \"" << expectedMsg << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(errorObserver->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  errorObserver->Clear()

#define CHECK_WARNING_MSG(warningObserver, msg, status)      \
  { \
  std::string expectedMsg(msg); \
  if (!warningObserver->GetWarning()) \
  { \
    std::cout << "Failed to catch any warning. Expected the warning message to contain \"" << expectedMsg << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(warningObserver->GetWarningMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Warning message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  warningObserver->Clear()

template<typename T> int TestSubdivision();

int UnitTestSubdivisionFilters (int, char*[])
{
  int status = EXIT_SUCCESS;

  status += TestSubdivision<vtkButterflySubdivisionFilter>();
  status += TestSubdivision<vtkLinearSubdivisionFilter>();
  status += TestSubdivision<vtkLoopSubdivisionFilter>();

  return status;
}

template<typename T> int TestSubdivision()
{
  int status = EXIT_SUCCESS;

  // Start of test
  vtkSmartPointer<T> subdivision0 =
    vtkSmartPointer<T>::New();
  std::cout << "Testing " << subdivision0->GetClassName() << std::endl;

  // Empty Print
  std::cout << "  Testing empty print...";
  std::ostringstream emptyPrint;
  subdivision0->Print(emptyPrint);
  std::cout << "PASSED" << std::endl;

  // Catch empty input error message
  std::cout << "  Testing empty input...";
  vtkSmartPointer<vtkTest::ErrorObserver>  executiveObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  subdivision0->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, executiveObserver);
  subdivision0->Update();

  int status1 = 0;
  CHECK_ERROR_MSG(executiveObserver,
                  "has 0 connections but is not optional.",
                  status1);
  if (status1 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  // Testing empty dataset
  std::cout << "  Testing empty dataset...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  subdivision0->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  subdivision0->SetInputData(polyData);
  subdivision0->SetNumberOfSubdivisions(4);
  subdivision0->Update();

  int status2 = 0;
  CHECK_ERROR_MSG(errorObserver,
                  "No data to subdivide",
                  status2);
  if (status2 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  // Create a triangle
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint ( 1.0, 0.0, 0.0 );
  points->InsertNextPoint ( 0.0, 0.0, 0.0 );
  points->InsertNextPoint ( 0.0, 1.0, 0.0 );
  points->InsertNextPoint ( 0.0, 1.0, 1.0 );
  points->InsertNextPoint ( 0.0, 1.0, -1.0 );

  vtkSmartPointer<vtkTriangle> triangle =
    vtkSmartPointer<vtkTriangle>::New();
  triangle->GetPointIds()->SetId ( 0, 0 );
  triangle->GetPointIds()->SetId ( 1, 1 );
  triangle->GetPointIds()->SetId ( 2, 2 );

  vtkSmartPointer<vtkCellArray> triangles =
    vtkSmartPointer<vtkCellArray>::New();
  triangles->InsertNextCell ( triangle );

  vtkSmartPointer<vtkPolyData> trianglePolyData =
    vtkSmartPointer<vtkPolyData>::New();
  trianglePolyData->SetPoints ( points );
  trianglePolyData->SetPolys ( triangles );

  std::cout << "  Testing a triangle...";
  subdivision0->SetInputData(trianglePolyData);
  subdivision0->Update();
  std::cout << "PASSED" << std::endl;

  std::cout << "  Testing non-manifold dataset...";

  vtkSmartPointer<vtkTriangle> triangle2 =
    vtkSmartPointer<vtkTriangle>::New();
  triangle2->GetPointIds()->SetId ( 0, 0 );
  triangle2->GetPointIds()->SetId ( 1, 1 );
  triangle2->GetPointIds()->SetId ( 2, 3 );
  vtkSmartPointer<vtkTriangle> triangle3 =
    vtkSmartPointer<vtkTriangle>::New();
  triangle3->GetPointIds()->SetId ( 0, 0 );
  triangle3->GetPointIds()->SetId ( 1, 1 );
  triangle3->GetPointIds()->SetId ( 2, 4 );

  triangles->InsertNextCell ( triangle2 );
  triangles->InsertNextCell ( triangle3 );
  triangles->Modified();

  vtkSmartPointer<vtkPolyData> nonManifoldPolyData =
    vtkSmartPointer<vtkPolyData>::New();
  nonManifoldPolyData->SetPoints ( points );
  nonManifoldPolyData->SetPolys ( triangles );

  subdivision0->SetInputData(nonManifoldPolyData);
  subdivision0->Modified();
  subdivision0->Update();
  int status3 = 0;
  CHECK_ERROR_MSG(errorObserver,
                  "Dataset is non-manifold and cannot be subdivided",
                  status3);
  if (status3 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  std::cout << "  Testing non-triangles...";
  vtkSmartPointer<vtkQuad> quad =
    vtkSmartPointer<vtkQuad>::New();
  quad->GetPointIds()->SetId(0,0);
  quad->GetPointIds()->SetId(1,1);
  quad->GetPointIds()->SetId(2,2);
  quad->GetPointIds()->SetId(3,3);

  vtkSmartPointer<vtkCellArray> cells =
    vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell ( triangle );
  cells->InsertNextCell ( quad );

  vtkSmartPointer<vtkPolyData> mixedPolyData =
    vtkSmartPointer<vtkPolyData>::New();
  mixedPolyData->SetPoints ( points );
  mixedPolyData->SetPolys ( cells );
  subdivision0->SetInputData(mixedPolyData);
  subdivision0->Update();

  int status4 = 0;
  CHECK_ERROR_MSG(errorObserver,
                  "only operates on triangles, but this data set has other cell types present",
                  status4);
  if (status4 == 0)
  {
    std::cout << "PASSED" << std::endl;
  }
  else
  {
    status++;
    std::cout << "FAILED" << std::endl;
  }

  std::cout << "PASSED" << std::endl;
  // End of test
  if (status)
  {
    std::cout << subdivision0->GetClassName() << " FAILED" << std::endl;
  }
  else
  {
    std::cout << subdivision0->GetClassName() << " PASSED" << std::endl;
  }

  return status;
}
