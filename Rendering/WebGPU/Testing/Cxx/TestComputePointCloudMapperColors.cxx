// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test renders a point cloud with colors with the WebGPU compute API and ensures that the
 * resulting image is correct
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

//------------------------------------------------------------------------------
int TestComputePointCloudMapperColors(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);
  renWin->Initialize();

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);

  vtkNew<vtkPoints> points;
  constexpr int sizeX = 100;
  constexpr int sizeY = 100;
  constexpr int sizeZ = 100;
  // 'divider' controls the space between the points. Higher values means points closer together
  constexpr float divider = 20.0f;
  constexpr float maxX = sizeX / divider;
  constexpr float maxY = sizeY / divider;
  constexpr float maxZ = sizeZ / divider;
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        int pointIndex = k + j * sizeZ + i * sizeY * sizeZ;

        points->InsertNextPoint(i / divider, j / divider, k / divider);
        colors->InsertComponent(
          pointIndex, 0, static_cast<unsigned char>(i / divider / maxX * 255.0f));
        colors->InsertComponent(
          pointIndex, 1, static_cast<unsigned char>(j / divider / maxY * 255.0f));
        colors->InsertComponent(
          pointIndex, 2, static_cast<unsigned char>(k / divider / maxZ * 127.0f + 127.0f));
        colors->InsertComponent(pointIndex, 3, 255);
      }
    }
  }

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);
  polydata->GetPointData()->SetScalars(colors);
  renderer->ResetCamera(polydata->GetBounds());

  vtkNew<vtkWebGPUComputePointCloudMapper> pointCloudMapper;
  pointCloudMapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->SetMapper(pointCloudMapper);

  renderer->AddActor(actor);

  int retVal = vtkRegressionTestImage(renWin);

  return !retVal;
}
