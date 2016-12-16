/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestPMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkPMaskPoints.h"

#include "vtkPolyData.h"
#include "vtkPoints.h"

#include "vtkMPIController.h"

#include "vtkCommand.h"
#include "vtkTestErrorObserver.h"
#include "vtkMathUtilities.h"

// MPI include
#include <mpi.h>

#include <cstdio>
#include <sstream>
#include <algorithm>

static vtkSmartPointer<vtkPolyData> MakePolyData(
  unsigned int numPoints);

int UnitTestPMaskPoints (int argc, char* argv[])
{
  int status = 0;

  // Test empty input
  // std::cout << "Testing empty input...";
  std::ostringstream print0;
  vtkSmartPointer<vtkPMaskPoints> mask0 =
    vtkSmartPointer<vtkPMaskPoints>::New();
  // For coverage
  mask0->SetController(NULL); mask0->SetController(NULL);
  mask0->Print(print0);

  vtkMPIController* cntrl = vtkMPIController::New();
  cntrl->Initialize( &argc, &argv, 0 );
  vtkMultiProcessController::SetGlobalController( cntrl );

  mask0->SetController(vtkMultiProcessController::GetGlobalController());

  mask0->SetInputData(MakePolyData(10000));
  mask0->GenerateVerticesOn();
  mask0->SetMaximumNumberOfPoints(99);
  mask0->ProportionalMaximumNumberOfPointsOn();
  mask0->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  mask0->Update();

  mask0->RandomModeOn();
  mask0->SetRandomModeType(0);
  mask0->Update();

  mask0->SetRandomModeType(1);
  mask0->Update();

  mask0->SetRandomModeType(2);
  mask0->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  mask0->Update();

  mask0->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);
  mask0->Update();

  mask0->SetRandomModeType(3);
  mask0->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  mask0->SingleVertexPerCellOn();
  mask0->Update();

  mask0->Print(print0);

  cntrl->Finalize();
  cntrl->Delete();
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
