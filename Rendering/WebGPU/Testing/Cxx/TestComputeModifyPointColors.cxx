// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test ensures that a compute pipeline is able to modify the existing render data of a render
 * pipeline.
 * This is tested by turning the colors of the 3 vertices of a triangle black using a compute
 * pipeline.
 */

#include "TestComputeModifyPointColorsShader.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

int TestComputeModifyPointColors(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);
  // Initialize() call necessary when a WebGPU compute class is going to use resources from the
  // render window/renderer/mapper.
  //
  // The modify point colors pipeline uses the render buffer of the WebGPUMapper. The pipeline is
  // then added to the renderer (which is a renderer which uses the resources of the render window).
  // Initialize() is thus necessary.
  renWin->Initialize();

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertPoint(0, -1, -1, 0);
  points->InsertPoint(1, 0, 1.5, 0);
  points->InsertPoint(2, 1, -1, 0);
  polydata->SetPoints(points);
  vtkNew<vtkCellArray> triangle;
  triangle->InsertNextCell({ 0, 2, 1 });
  polydata->SetPolys(triangle);

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
  polydata->GetPointData()->SetScalars(colors);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Getting the WebGPUMapper to access the point attribute render buffers
  vtkWebGPUPolyDataMapper* webGPUMapper = vtkWebGPUPolyDataMapper::SafeDownCast(mapper);

  // Getting the point colors render buffer and indicating that we want it to be bound to (0,
  // 0) and the uniform buffer to be bound to (0, 1)
  int bufferGroup = 0;
  int bufferBinding = 0;
  int uniformsGroup = 0;
  int uniformsBinding = 1;

  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> pointColorsRenderBuffer =
    webGPUMapper->AcquirePointAttributeComputeRenderBuffer(
      vtkWebGPUPolyDataMapper::PointDataAttributes::POINT_COLORS, bufferGroup, bufferBinding,
      uniformsGroup, uniformsBinding);
  // Label in case something goes wrong
  pointColorsRenderBuffer->SetLabel("Point colors render buffer");

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> dynamicColorsComputePipeline;

  // Creating the compute pass
  vtkSmartPointer<vtkWebGPUComputePass> dynamicColorsComputePass =
    dynamicColorsComputePipeline->CreateComputePass();
  dynamicColorsComputePass->SetShaderSource(TestComputeModifyPointColorsShader);
  dynamicColorsComputePass->SetShaderEntryPoint("changePointColorCompute");
  // Adding the render buffer
  dynamicColorsComputePass->AddRenderBuffer(pointColorsRenderBuffer);
  int nbGroupsX = std::ceil(polydata->GetPointData()->GetNumberOfTuples() / 32.0f);
  dynamicColorsComputePass->SetWorkgroups(nbGroupsX, 1, 1);

  // Adding the compute pipeline to the renderer.
  // The pipeline will be executed each frame before the rendering pass
  vtkWebGPURenderer* wegpuRenderer =
    vtkWebGPURenderer::SafeDownCast(renWin->GetRenderers()->GetFirstRenderer());
  wegpuRenderer->AddPreRenderComputePipeline(dynamicColorsComputePipeline);

  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();

  // Screenshot taken by the regression testing isn't flipped.
  // This isn't an issue for testing but that may be something to look into
  int retVal = vtkRegressionTestImage(renWin);

  return !retVal;
}
