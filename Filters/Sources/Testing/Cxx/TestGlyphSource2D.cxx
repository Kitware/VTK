// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkGlyphSource2D.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

int TestGlyphSource2D(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkGlyphSource2D> glyphSource = vtkSmartPointer<vtkGlyphSource2D>::New();
  glyphSource->SetColor(1.0, 1.0, 1.0);
  glyphSource->CrossOff();
  glyphSource->DashOff();
  glyphSource->FilledOn();
  glyphSource->SetGlyphTypeToVertex();

  glyphSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  glyphSource->SetCenter(center);

  randomSequence->Next();
  double rotationAngle = randomSequence->GetValue();
  glyphSource->SetRotationAngle(rotationAngle);

  randomSequence->Next();
  double scale = randomSequence->GetValue();
  glyphSource->SetScale(scale);

  glyphSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = glyphSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if (points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  glyphSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for (unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
  }
  glyphSource->SetCenter(center);

  randomSequence->Next();
  rotationAngle = randomSequence->GetValue();
  glyphSource->SetRotationAngle(rotationAngle);

  randomSequence->Next();
  scale = randomSequence->GetValue();
  glyphSource->SetScale(scale);

  glyphSource->Update();

  polyData = glyphSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  // Test VTK_ARROW_GLYPH
  glyphSource->SetGlyphTypeToArrow();
  glyphSource->FilledOff();
  center[0] = 0.0;
  center[1] = 0.0;
  center[2] = 0.0;
  glyphSource->SetCenter(center);
  glyphSource->SetRotationAngle(0.0);
  glyphSource->SetScale(1.0);

  // Test tip length
  glyphSource->SetTipLength(0.2);
  glyphSource->Update();
  polyData = glyphSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetNumberOfPoints() != 5)
  {
    std::cerr << "Wrong number of points. Expected 5 but got " << points->GetNumberOfPoints() << '.'
              << std::endl;
    return EXIT_FAILURE;
  }

  double coords[3] = { 0.0, 0.0, 0.0 };
  points->GetPoint(2, coords);
  if (coords[0] != 0.3 || coords[1] != -0.1 || coords[2] != 0.0)
  {
    std::cerr << "Wrong coordinate for point 2. Expected (0.3, -0.1, 0.0) but got (" << coords[0]
              << ", " << coords[1] << ", " << coords[2] << ')' << std::endl;
    return EXIT_FAILURE;
  }

  // Test double tip
  glyphSource->SetDoublePointed(true);
  glyphSource->Update();
  polyData = glyphSource->GetOutput();
  points = polyData->GetPoints();

  if (points->GetNumberOfPoints() != 8)
  {
    std::cerr << "Wrong number of points. Expected 8 but got " << points->GetNumberOfPoints() << '.'
              << std::endl;
    return EXIT_FAILURE;
  }

  points->GetPoint(7, coords);
  if (coords[0] != -0.3 || coords[1] != 0.1 || coords[2] != 0.0)
  {
    std::cerr << "Wrong coordinate for point 7. Expected (-0.3, 0.1, 0.0) but got (" << coords[0]
              << ", " << coords[1] << ", " << coords[2] << ')' << std::endl;
    return EXIT_FAILURE;
  }

  // Test tips pointing inwards
  glyphSource->SetPointInwards(true);
  glyphSource->Update();
  polyData = glyphSource->GetOutput();
  points = polyData->GetPoints();

  points->GetPoint(0, coords);
  if (coords[0] != -0.3 || coords[1] != 0.0 || coords[2] != 0.0)
  {
    std::cerr << "Wrong coordinate for point 0. Expected (-0.3, 0.0, 0.0) but got (" << coords[0]
              << ", " << coords[1] << ", " << coords[2] << ')' << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
