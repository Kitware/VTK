/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkLineSource.h>
#include <vtkLogger.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>
#include <vtkVectorOperators.h>

int TestLineSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
  lineSource->SetResolution(8);

  lineSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double point1[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint1(point1);

  double point2[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint2(point2);

  lineSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = lineSource->GetOutput();
  vtkSmartPointer<vtkPoints> outputPoints = polyData->GetPoints();

  if (outputPoints->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  lineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint1(point1);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  lineSource->SetPoint2(point2);

  lineSource->Update();

  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();

  if (outputPoints->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  lineSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
  inputPoints->SetDataType(VTK_DOUBLE);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point1);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point2);

  lineSource->SetPoints(inputPoints);

  lineSource->Update();

  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();

  if (outputPoints->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  inputPoints->Reset();

  lineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point1[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point1);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    point2[i] = randomSequence->GetValue();
  }
  inputPoints->InsertNextPoint(point2);

  lineSource->SetPoints(inputPoints);

  lineSource->Update();

  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();
  if (outputPoints->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  lineSource->SetPoints(nullptr);
  lineSource->SetPoint1(0, 0, 0);
  lineSource->SetPoint2(1, 1, 2);
  lineSource->SetNumberOfRefinementRatios(3);
  lineSource->SetRefinementRatio(0, 0.1);
  lineSource->SetRefinementRatio(1, 0.7);
  lineSource->SetRefinementRatio(2, 1.0);
  lineSource->SetUseRegularRefinement(false);
  lineSource->SetResolution(10);
  lineSource->Update();
  polyData = lineSource->GetOutput();
  outputPoints = polyData->GetPoints();
  if (outputPoints->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  if (outputPoints->GetNumberOfPoints() != 3)
  {
    vtkLogF(ERROR, "incorrect number of points (expected 3: got: %d)",
      static_cast<int>(outputPoints->GetNumberOfPoints()));
    return EXIT_FAILURE;
  }

  const vtkVector3d expected(0.7, 0.7, 1.4);
  vtkVector3d pt;
  outputPoints->GetPoint(1, pt.GetData());
  if (pt != expected)
  {
    vtkLogF(ERROR, "incorrect point (expected (%g, %g, %g): got: (%g, %g, %g))", expected[0],
      expected[1], expected[2], pt[0], pt[1], pt[2]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
