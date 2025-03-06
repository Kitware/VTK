// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test renders a point cloud with additional quad with the WebGPU compute API and ensures that
 * the quad occlude/hide parts of the point cloud
 */

#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWebGPUComputePointCloudMapper.h"
#include "vtkWebGPURenderer.h"

#include <numeric> // for std::iota

namespace
{
vtkNew<vtkPolyData> CreateQuadPolydata(double translateX, double translateY, double translateZ)
{
  vtkNew<vtkPolyData> quadPolydata;

  vtkNew<vtkPoints> quadPoints;
  quadPoints->InsertPoint(0, -5 + translateX, -5 + translateY, 5 + translateZ);
  quadPoints->InsertPoint(1, 5 + translateX, -5 + translateY, 5 + translateZ);
  quadPoints->InsertPoint(2, -5 + translateX, 5 + translateY, 5 + translateZ);
  quadPoints->InsertPoint(3, 5 + translateX, 5 + translateY, 5 + translateZ);
  quadPolydata->SetPoints(quadPoints);

  vtkNew<vtkCellArray> quad;
  quad->InsertNextCell({ 0, 1, 3, 2 });
  quadPolydata->SetPolys(quad);

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(3);
  colors->InsertComponent(0, 0, 255);
  colors->InsertComponent(0, 1, 0);
  colors->InsertComponent(0, 2, 0);
  colors->InsertComponent(0, 3, 255);
  colors->InsertComponent(1, 0, 0);
  colors->InsertComponent(1, 1, 255);
  colors->InsertComponent(1, 2, 0);
  colors->InsertComponent(1, 3, 255);
  colors->InsertComponent(2, 0, 0);
  colors->InsertComponent(2, 1, 0);
  colors->InsertComponent(2, 2, 255);
  colors->InsertComponent(2, 3, 255);
  colors->InsertComponent(3, 0, 255);
  colors->InsertComponent(3, 1, 255);
  colors->InsertComponent(3, 2, 0);
  colors->InsertComponent(3, 3, 255);
  quadPolydata->GetPointData()->SetScalars(colors);

  return quadPolydata;
}
} // namespace

//------------------------------------------------------------------------------
int TestComputePointCloudMapperGeometry(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);
  renWin->Initialize();

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

  vtkNew<vtkPoints> points;
  constexpr int sizeX = 100;
  constexpr int sizeY = 100;
  constexpr int sizeZ = 100;
  // 'divider' controls the space between the points. Higher values means points closer together
  constexpr float divider = 10.0f;
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        points->InsertNextPoint(i / divider, j / divider, k / divider);
      }
    }
  }

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);
  renderer->ResetCamera(polydata->GetBounds());

  vtkNew<vtkWebGPUComputePointCloudMapper> pointCloudMapper;
  pointCloudMapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->SetMapper(pointCloudMapper);

  renderer->AddActor(actor);

  double* position = renderer->GetActiveCamera()->GetPosition();
  renderer->GetActiveCamera()->SetPosition(position[0], position[1], position[2] + 10);

  vtkNew<vtkPolyData> quadPolydata = CreateQuadPolydata(0, 0, 0);
  vtkNew<vtkPolyData> quadPolydata2 = CreateQuadPolydata(10, 10, 5);

  vtkNew<vtkPolyDataMapper> quadMapper;
  vtkNew<vtkPolyDataMapper> quadMapper2;
  quadMapper->SetInputData(quadPolydata);
  quadMapper2->SetInputData(quadPolydata2);

  vtkNew<vtkActor> quadActor;
  vtkNew<vtkActor> quadActor2;
  quadActor->SetMapper(quadMapper);
  quadActor2->SetMapper(quadMapper2);

  renderer->AddActor(quadActor);
  renderer->AddActor(quadActor2);

  int retVal = vtkRegressionTestImage(renWin);

  return !retVal;
}
