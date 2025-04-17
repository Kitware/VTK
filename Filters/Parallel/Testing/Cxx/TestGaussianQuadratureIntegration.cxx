// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkExtractGeometry.h"
#include "vtkIntegrateAttributes.h"
#include "vtkIntegrationGaussianStrategy.h"
#include "vtkIntegrationLinearStrategy.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkPlane.h"
#include "vtkUnstructuredGrid.h"

double GetVolume(vtkIntegrateAttributes* integrator)
{
  return integrator->GetOutput()->GetCellData()->GetArray(0)->GetTuple1(0);
}

bool Assert(bool test, const std::string& msg)
{
  if (!test)
  {
    vtkLog(ERROR, "Test failed: " << msg);
  }
  return test;
}

bool TestQuad(vtkIntegrateAttributes* linearIntegrator, vtkIntegrateAttributes* gaussianIntegrator)
{
  bool test = true;
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_QUAD);
  source->SetBlocksDimensions(1, 1, 1);
  source->Update();

  linearIntegrator->SetInputConnection(source->GetOutputPort());
  linearIntegrator->Update();
  gaussianIntegrator->SetInputConnection(source->GetOutputPort());
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(
                   GetVolume(linearIntegrator), GetVolume(gaussianIntegrator), 1e-10) &&
      vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), 1.0 * 1.0, 1e-10),
    "QUAD, Planar quad area should be the same for both Gaussian and linear integration.");

  // Make non planar quad
  vtkSmartPointer<vtkUnstructuredGrid> usg = source->GetOutput();
  vtkPoints* points;
  points = usg->GetPoints();
  double coord[3];
  points->GetPoint(0, coord);
  coord[2] = 1;
  points->SetPoint(0, coord);
  usg->SetPoints(points);
  gaussianIntegrator->SetInputData(usg);
  gaussianIntegrator->Update();

  test &= Assert(GetVolume(gaussianIntegrator) == 1.2809241071215176078,
    "QUAD, Wrong Gaussian integration volume for non planar shape");

  return test;
}

bool TestHex(vtkIntegrateAttributes* linearIntegrator, vtkIntegrateAttributes* gaussianIntegrator)
{
  bool test = true;
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_HEXAHEDRON);
  source->SetBlocksDimensions(1, 1, 1);
  source->Update();

  linearIntegrator->SetInputConnection(source->GetOutputPort());
  linearIntegrator->Update();
  gaussianIntegrator->SetInputConnection(source->GetOutputPort());
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(
                   GetVolume(linearIntegrator), GetVolume(gaussianIntegrator), 1e-10) &&
      vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), 1.0 * 1.0 * 1.0, 1e-10),
    "HEXAHEDRON, Standard Hexahedron volume should be the same for both Gaussian and linear "
    "integration.");

  // Make non planar faces
  vtkSmartPointer<vtkUnstructuredGrid> usg = source->GetOutput();
  vtkPoints* points;
  points = usg->GetPoints();
  double coord[3];
  points->GetPoint(0, coord);
  coord[0] = -1;
  points->SetPoint(0, coord);
  usg->SetPoints(points);
  gaussianIntegrator->SetInputData(usg);
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), 1.25, 1e-10),
    "HEXAHEDRON, Wrong Gaussian integration volume for non planar shape");

  return test;
}

bool TestWedge(vtkIntegrateAttributes* linearIntegrator, vtkIntegrateAttributes* gaussianIntegrator)
{
  bool test = true;
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_WEDGE);
  source->SetBlocksDimensions(1, 1, 1);
  source->Update();

  vtkNew<vtkPlane> plane;
  double origin[3] = { 1.0, 1.0, 0.0 };
  plane->SetOrigin(origin);
  double normal[3] = { 0.5, 0.5, -0.5 };
  plane->SetNormal(normal);

  vtkNew<vtkExtractGeometry> extractor;
  extractor->SetImplicitFunction(plane);
  extractor->SetInputConnection(source->GetOutputPort());

  linearIntegrator->SetInputConnection(extractor->GetOutputPort());
  linearIntegrator->Update();
  gaussianIntegrator->SetInputConnection(extractor->GetOutputPort());
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(
                   GetVolume(linearIntegrator), GetVolume(gaussianIntegrator), 1e-10) &&
      vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), 1.0 * 1.0 * 0.5 * 1.0, 1e-10),
    "WEDGE, Standard wedge volume should be the same for both Gaussian and linear integration.");

  // Make non planar faces
  vtkSmartPointer<vtkUnstructuredGrid> usg = extractor->GetOutput();
  vtkPoints* points;
  points = usg->GetPoints();
  double coord[3];
  points->GetPoint(0, coord);
  coord[0] = -1;
  points->SetPoint(0, coord);
  usg->SetPoints(points);
  gaussianIntegrator->SetInputData(usg);
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), 0.75, 1e-10),
    "WEDGE, Wrong Gaussian integration volume for non planar shape");

  return test;
}

bool TestPyramid(
  vtkIntegrateAttributes* linearIntegrator, vtkIntegrateAttributes* gaussianIntegrator)
{
  bool test = true;
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_PYRAMID);
  source->SetBlocksDimensions(1, 1, 1);
  source->Update();

  vtkNew<vtkPlane> plane;
  double origin[3] = { 0.55, 0.0, 0.0 };
  plane->SetOrigin(origin);
  double normal[3] = { 1.0, 0.0, 0.0 };
  plane->SetNormal(normal);

  vtkNew<vtkExtractGeometry> extractor;
  extractor->SetImplicitFunction(plane);
  extractor->SetInputConnection(source->GetOutputPort());

  linearIntegrator->SetInputConnection(extractor->GetOutputPort());
  linearIntegrator->Update();
  gaussianIntegrator->SetInputConnection(extractor->GetOutputPort());
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(
                   GetVolume(linearIntegrator), GetVolume(gaussianIntegrator), 1e-10) &&
      vtkMathUtilities::FuzzyCompare(
        GetVolume(gaussianIntegrator), 1.0 * 1.0 * 1.0 / 3.0 * 0.5, 1e-10),
    "PYRAMID, Standard Pyramid volume should be the same for both Gaussian and linear "
    "integration.");

  // Make non planar faces
  vtkSmartPointer<vtkUnstructuredGrid> usg = extractor->GetOutput();
  vtkPoints* points;
  points = usg->GetPoints();
  double coord[3];
  points->GetPoint(0, coord);
  coord[0] = -1;
  points->SetPoint(0, coord);
  usg->SetPoints(points);
  gaussianIntegrator->SetInputData(usg);
  gaussianIntegrator->Update();
  double gaussVolume = GetVolume(gaussianIntegrator);

  test &= Assert(vtkMathUtilities::FuzzyCompare(gaussVolume, 0.25, 1e-10),
    "PYRAMID, Wrong Gaussian integration volume for non planar shape");

  // Change points ordering
  vtkPoints* pts;
  pts = usg->GetPoints();
  double pt0[3];
  double pt1[3];
  double pt2[3];
  double pt3[3];
  double pt4[3];
  pts->GetPoint(0, pt0);
  pts->GetPoint(1, pt1);
  pts->GetPoint(2, pt2);
  pts->GetPoint(3, pt3);
  pts->GetPoint(4, pt4);

  pts->SetPoint(0, pt2);
  pts->SetPoint(1, pt0);
  pts->SetPoint(2, pt3);
  pts->SetPoint(3, pt1);
  pts->SetPoint(4, pt4);
  usg->SetPoints(pts);
  usg->Modified();
  gaussianIntegrator->SetInputData(usg);
  gaussianIntegrator->Update();

  test &= Assert(vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), 0.25, 1e-10),
    "PYRAMID, Wrong Gaussian integration volume for non planar shape");
  test &= Assert(vtkMathUtilities::FuzzyCompare(GetVolume(gaussianIntegrator), gaussVolume, 1e-10),
    "PYRAMID, Gauss Integration should be independant of point ordering");

  return test;
}

int TestGaussianQuadratureIntegration(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkIntegrateAttributes> linearIntegrator;
  linearIntegrator->SetIntegrationStrategy(vtkNew<vtkIntegrationLinearStrategy>());
  vtkNew<vtkIntegrateAttributes> gaussianIntegrator;
  gaussianIntegrator->SetIntegrationStrategy(vtkNew<vtkIntegrationGaussianStrategy>());

  bool testVal = TestQuad(linearIntegrator, gaussianIntegrator);
  testVal &= TestHex(linearIntegrator, gaussianIntegrator);
  testVal &= TestWedge(linearIntegrator, gaussianIntegrator);
  testVal &= TestPyramid(linearIntegrator, gaussianIntegrator);

  return testVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
