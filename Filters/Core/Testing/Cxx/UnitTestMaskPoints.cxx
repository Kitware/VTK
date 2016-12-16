/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkMaskPoints.h"

#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkImageData.h"

#include "vtkCommand.h"
#include "vtkTestErrorObserver.h"
#include "vtkMathUtilities.h"

#include <cstdio>
#include <sstream>
#include <algorithm>

static vtkSmartPointer<vtkPolyData> MakePolyData(
  unsigned int numPoints);
static vtkSmartPointer<vtkImageData> MakeImageData(
  unsigned int dim);

int UnitTestMaskPoints (int, char*[])
{
  int status = 0;

  // Test empty input
  std::cout << "Testing empty input...";
  std::ostringstream print0;
  vtkSmartPointer<vtkMaskPoints> mask0 =
    vtkSmartPointer<vtkMaskPoints>::New();
  mask0->Print(print0);
  std::cout << "PASSED" << std::endl;;

  std::cout << "Testing defaults...";
  mask0->SetInputData(MakePolyData(10000));
  mask0->GenerateVerticesOn();
  mask0->SetMaximumNumberOfPoints(101);
  mask0->SetOnRatio(10);
  mask0->SetOffset (100);
  mask0->ProportionalMaximumNumberOfPointsOn();
  mask0->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  mask0->Update();
  if (mask0->GetOutput()->GetNumberOfPoints() != 102)
  {
    std::cout << "FAILED: Expected 102"
               << " but got " << mask0->GetOutput()->GetNumberOfPoints() << std::endl;
    status++;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  std::cout << "Testing RandomModeType(0)...";
  mask0->SetInputData(MakePolyData(10000));
  mask0->RandomModeOn();
  mask0->SetRandomModeType(0);
  mask0->SetMaximumNumberOfPoints(99);
  mask0->SetOffset (0);
  mask0->Update();
  if (mask0->GetOutput()->GetNumberOfPoints() != mask0->GetMaximumNumberOfPoints())
  {
   std::cout << "FAILED: Expected " << mask0->GetMaximumNumberOfPoints()
             << " but got " << mask0->GetOutput()->GetNumberOfPoints() << std::endl;
   status++;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  std::cout << "Testing RandomModeType(1)...";
  mask0->SetRandomModeType(1);
  mask0->Update();
  if (mask0->GetOutput()->GetNumberOfPoints() != mask0->GetMaximumNumberOfPoints())
  {
   std::cout << "FAILED: Expected " << mask0->GetMaximumNumberOfPoints()
             << " but got " << mask0->GetOutput()->GetNumberOfPoints() << std::endl;
   status++;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  std::cout << "Testing RandomModeType(1)...";
  mask0->SetRandomModeType(2);
  mask0->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  mask0->Update();
  if (mask0->GetOutput()->GetNumberOfPoints() != mask0->GetMaximumNumberOfPoints())
  {
   std::cout << "FAILED: Expected " << mask0->GetMaximumNumberOfPoints()
             << " but got " << mask0->GetOutput()->GetNumberOfPoints() << std::endl;
   status++;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  std::cout << "Testing with image data...";
  // Try with image data
  vtkSmartPointer<vtkMaskPoints> mask1 =
    vtkSmartPointer<vtkMaskPoints>::New();
  mask1->SetInputData(MakeImageData(10));
  mask1->GenerateVerticesOn();
  mask1->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  mask1->RandomModeOn();
  mask1->SetRandomModeType(2);
  mask1->SetMaximumNumberOfPoints(50);
  mask1->Update();
  if (mask1->GetOutput()->GetNumberOfPoints() != mask1->GetMaximumNumberOfPoints())
  {
    std::cout << "FAILED: Expected " << mask1->GetMaximumNumberOfPoints()
             << " but got " << mask1->GetOutput()->GetNumberOfPoints() << std::endl;
    status++;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  // Print an initialized object
  mask0->Print(print0);

  // Error conditions
  std::cout << "Testing Error conditions...";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  mask0->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  mask0->SetInputData(MakePolyData(0));
  mask0->Update();
  int status1 = errorObserver->CheckErrorMessage("No points to mask");
  if (status1)
  {
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  // Suppress the debug output
  vtkObject::GlobalWarningDisplayOff();

  std::cout << "Testing SingleVertexPerCell...";
  mask0->SetInputData(MakePolyData(1000));
  mask0->SetRandomModeType(3);
  mask0->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  mask0->SingleVertexPerCellOn();
  mask0->DebugOn();
  mask0->Update();
  if (mask0->GetOutput()->GetNumberOfCells() !=
      mask0->GetOutput()->GetNumberOfPoints())
  {
     std::cout << "FAILED: Expected " << mask0->GetOutput()->GetNumberOfPoints()
             << " but got " << mask0->GetOutput()->GetNumberOfCells() << std::endl;
   status++;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  if (status)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

vtkSmartPointer<vtkPolyData> MakePolyData(unsigned int numPoints)
{
  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  std::vector<double> line;
  for (unsigned int i = 0; i < numPoints; ++i)
  {
    line.push_back(static_cast<double>(i));
  }
  std::random_shuffle ( line.begin(), line.end() );
  for (unsigned int i = 0; i < numPoints; ++i)
  {
    points->InsertNextPoint(line[i], 0.0, 0.0);
  }
  polyData->SetPoints(points);
  return polyData;
}

vtkSmartPointer<vtkImageData> MakeImageData(unsigned int dim)
{
  vtkSmartPointer<vtkImageData> imageData =
    vtkSmartPointer<vtkImageData>::New();
  imageData->SetDimensions(dim, dim, 1);
  imageData->AllocateScalars(VTK_UNSIGNED_CHAR,1);
  for(unsigned int x = 0; x < dim; ++x)
  {
    for(unsigned int y = 0; y < dim; ++y)
    {
      unsigned char* pixel =
        static_cast<unsigned char*>(imageData->GetScalarPointer(x,y,0));
      pixel[0] = static_cast<unsigned char>(x + y);
    }
  }

  return imageData;
}
