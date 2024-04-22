// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include <vtkActor.h>
#include <vtkContourFilter.h>
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkImplicitArray.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <cstdlib>

namespace
{

struct SphereLevelSetBackend
{
  SphereLevelSetBackend(vtkDataSet* grid, double radius)
    : Grid(grid)
    , Radius(radius)
  {
  }
  double operator()(int idx) const
  {
    double pt[3];
    this->Grid->GetPoint(idx, pt);
    return vtkMath::Norm(pt) - this->Radius;
  }
  vtkDataSet* Grid = nullptr;
  double Radius = 0.0;
};

} // namespace

int TestContourImplicitArrays(int argc, char* argv[])
{
  vtkNew<vtkImageData> baseGrid;
  int nPix = 300;
  int halfCells = nPix / 2 - 1;
  baseGrid->SetExtent(-halfCells, halfCells, -halfCells, halfCells, -halfCells, halfCells);
  baseGrid->SetSpacing(1.0 / nPix, 1.0 / nPix, 1.0 / nPix);
  vtkNew<vtkImplicitArray<SphereLevelSetBackend>> levelSet;
  levelSet->SetName("LevelSet");
  levelSet->SetBackend(std::make_shared<SphereLevelSetBackend>(baseGrid, 0.60));
  levelSet->SetNumberOfComponents(1);
  levelSet->SetNumberOfTuples(std::pow(nPix, 3));
  baseGrid->GetPointData()->AddArray(levelSet);
  baseGrid->GetPointData()->SetActiveScalars("LevelSet");

  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(baseGrid);
  contour->SetContourValues({ 0.0 });

  contour->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(contour->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  renWin->Render();

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(9, 9, 9);
  renderer->ResetCamera();

  return (vtkRegressionTester::Test(argc, argv, renWin, 10) == vtkRegressionTester::PASSED)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
