// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include <vtkColorTransferFunction.h>
#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkImplicitArray.h>
#include <vtkMath.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include <cstdlib>

namespace
{

struct TorusLevelSetBackend
{
  TorusLevelSetBackend(vtkDataSet* grid, double majRadius, double minRadius)
    : Grid(grid)
    , MajRadius(majRadius)
    , MinRadius(minRadius)
  {
  }
  double operator()(int idx) const
  {
    double* pt = this->Grid->GetPoint(idx);
    return std::pow(std::sqrt(std::pow(pt[0], 2) + std::pow(pt[1], 2)) - this->MajRadius, 2) +
      std::pow(pt[2], 2) - std::pow(this->MinRadius, 2);
  }
  vtkDataSet* Grid = nullptr;
  double MajRadius = 0.25;
  double MinRadius = 0.1;
};

} // namespace

int TestSmartVolumeMapperImplicitArray(int argc, char* argv[])
{
  vtkNew<vtkImageData> baseGrid;
  int nPix = 100;
  int halfCells = nPix / 2 - 1;
  baseGrid->SetExtent(-halfCells, halfCells, -halfCells, halfCells, -halfCells, halfCells);
  baseGrid->SetSpacing(1.0 / nPix, 1.0 / nPix, 1.0 / nPix);
  vtkNew<vtkImplicitArray<::TorusLevelSetBackend>> levelSet;
  levelSet->SetName("LevelSet");
  levelSet->SetBackend(std::make_shared<::TorusLevelSetBackend>(baseGrid, 0.25, 0.2));
  levelSet->SetNumberOfComponents(1);
  levelSet->SetNumberOfTuples(std::pow(nPix, 3));
  baseGrid->GetPointData()->AddArray(levelSet);
  baseGrid->GetPointData()->SetActiveScalars("LevelSet");

  vtkNew<vtkSmartVolumeMapper> mapper;
  mapper->SetInputData(baseGrid);
  double scalarRange[2] = { 0.0 };
  mapper->GetInput()->GetScalarRange(scalarRange);
  mapper->SetBlendModeToComposite();
  mapper->SetAutoAdjustSampleDistances(1);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[1], 0.0);
  scalarOpacity->AddPoint(scalarRange[1] / 20, 0.0);
  scalarOpacity->AddPoint(scalarRange[0], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkColorTransferFunction* ctFunc = volumeProperty->GetRGBTransferFunction();
  ctFunc->RemoveAllPoints();
  ctFunc->AddRGBPoint(scalarRange[1], 86.0 / 255.0, 150.0 / 255.0, 158.0 / 255.0);
  ctFunc->AddRGBPoint(scalarRange[0], 246.0 / 255.0, 234.0 / 255.0, 194.0 / 255.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddViewProp(volume);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  renWin->Render();

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(0.3, 0.3, 1.0);
  renderer->ResetCamera();

  return (vtkRegressionTester::Test(argc, argv, renWin, 10) == vtkRegressionTester::PASSED)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
};
