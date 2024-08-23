// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test renders a point cloud behind another with the WebGPU compute API and ensures that the
 * point cloud that is rendered behind is occluded by the one in front
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkWebGPUComputePointCloudMapper.h"
#include "vtkWebGPURenderer.h"
#include "vtkXMLPolyDataReader.h"

#include <chrono>
#include <iostream>

namespace
{
vtkSmartPointer<vtkActor> CreatePointCube(
  double translationX, double translationY, double translationZ)
{
  vtkNew<vtkPolyData> polydata;

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);

  vtkNew<vtkPoints> points;
  constexpr int sizeX = 40;
  constexpr int sizeY = 40;
  constexpr int sizeZ = 40;
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

        points->InsertNextPoint(
          i / divider + translationX, j / divider + translationY, k / divider + translationZ);
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

  std::vector<vtkIdType> pointIndices(sizeX * sizeY * sizeZ);
  for (int i = 0; i < sizeX * sizeY * sizeZ; i++)
  {
    pointIndices[i] = i;
  }

  vtkNew<vtkCellArray> pointsCellArray;
  pointsCellArray->InsertNextCell(sizeX * sizeY * sizeZ, pointIndices.data());

  vtkSmartPointer<vtkWebGPUComputePointCloudMapper> mapper =
    vtkSmartPointer<vtkWebGPUComputePointCloudMapper>::New();

  polydata->SetPoints(points);
  polydata->SetPolys(pointsCellArray);
  polydata->GetPointData()->SetScalars(colors);

  mapper->SetInputData(polydata);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}
}

//------------------------------------------------------------------------------
int TestComputePointCloudMapperDepth(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  renWin->SetSize(512, 512);
  renWin->Initialize();

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkActor> actor = CreatePointCube(0, 0, 0);
  vtkSmartPointer<vtkActor> actor2 = CreatePointCube(0, 0, -10);

  renderer->AddActor(actor);
  renderer->AddActor(actor2);

  renderer->ResetCamera(actor->GetBounds());
  renderer->GetActiveCamera()->SetPosition(1.01544, 2.60141, 14.8666);
  renderer->ResetCameraClippingRange();

  int retVal = vtkRegressionTestImage(renWin);

  return !retVal;
}
