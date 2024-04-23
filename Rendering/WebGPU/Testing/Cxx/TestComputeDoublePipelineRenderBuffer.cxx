// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test ensures that two pipelines are able to modify the existing render data of a render
 * pipeline.
 *
 * This is tested by modifying the colors and positions of the 3 vertices of a triangle with one
 * rendered frame in between:
 *
 * - First pipeline modifies the colors
 * - Render
 * - Second pipeline modifies the positions
 * - Render
 * - Test expected result
 */

#include "TestComputeModifyPointColorsShader.h"
#include "TestComputeModifyPointPositionsShader.h"
#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

namespace
{
vtkSmartPointer<vtkWebGPUComputePipeline> SetupPointColorsPipeline(
  vtkWebGPUPolyDataMapper* webGPUMapper, vtkPolyData* polydata)
{
  // Getting the point colors render buffer and indicating that we want it to be bound to (0,
  // 0) and the uniform buffer to be bound to (0, 1)
  int colorsBufferGroup = 0;
  int colorsBufferBinding = 0;
  int colorsUniformsGroup = 0;
  int colorsUniformsBinding = 1;

  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> pointColorsRenderBuffer =
    webGPUMapper->AcquirePointAttributeComputeRenderBuffer(
      vtkWebGPUPolyDataMapper::PointDataAttributes::POINT_COLORS, colorsBufferGroup,
      colorsBufferBinding, colorsUniformsGroup, colorsUniformsBinding);
  // Label in case something goes wrong
  pointColorsRenderBuffer->SetLabel("Point colors render buffer");

  // Creating the compute pipeline
  int nbGroupsX = std::ceil(polydata->GetPointData()->GetNumberOfTuples() / 32.0f);

  vtkSmartPointer<vtkWebGPUComputePipeline> dynamicColorsCompute;
  dynamicColorsCompute = vtkSmartPointer<vtkWebGPUComputePipeline>::New();
  dynamicColorsCompute->SetShaderSource(TestComputeModifyPointColorsShader);
  dynamicColorsCompute->SetShaderEntryPoint("changePointColorCompute");
  dynamicColorsCompute->AddRenderBuffer(pointColorsRenderBuffer);
  dynamicColorsCompute->SetWorkgroups(nbGroupsX, 1, 1);

  return dynamicColorsCompute;
}

vtkSmartPointer<vtkWebGPUComputePipeline> SetupPointPositionsPipeline(
  vtkWebGPUPolyDataMapper* webGPUMapper, vtkPolyData* polydata)
{
  // This buffer is going to be bound in the shader of the second so this is a separate shader from
  // the first pipeline which means that we can use the same groups/bindings combinations without
  // having any issue.
  int positionsBufferGroup = 0;
  int positionsBufferBinding = 0;
  int positionsUniformsGroup = 0;
  int positionsUniformsBinding = 1;

  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> pointColorsRenderBuffer =
    webGPUMapper->AcquirePointAttributeComputeRenderBuffer(
      vtkWebGPUPolyDataMapper::PointDataAttributes::POINT_POSITIONS, positionsBufferGroup,
      positionsBufferBinding, positionsUniformsGroup, positionsUniformsBinding);
  // Label in case something goes wrong
  pointColorsRenderBuffer->SetLabel("Point positions render buffer");

  // Creating the compute pipeline
  int nbGroupsX = std::ceil(polydata->GetPointData()->GetNumberOfTuples() / 32.0f);

  vtkSmartPointer<vtkWebGPUComputePipeline> dynamicPositionsCompute;
  dynamicPositionsCompute = vtkSmartPointer<vtkWebGPUComputePipeline>::New();
  dynamicPositionsCompute->SetShaderSource(TestComputeModifyPointPositionsShader);
  dynamicPositionsCompute->SetShaderEntryPoint("changePointPositionCompute");
  dynamicPositionsCompute->AddRenderBuffer(pointColorsRenderBuffer);
  dynamicPositionsCompute->SetWorkgroups(nbGroupsX, 1, 1);

  return dynamicPositionsCompute;
}
}

int TestComputeDoublePipelineRenderBuffer(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

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
  renderer->SetBackground(0.2, 0.3, 0.4);

  // Getting the WebGPUMapper to access the point attribute render buffers
  vtkWebGPUPolyDataMapper* webGPUMapper = vtkWebGPUPolyDataMapper::SafeDownCast(mapper);

  vtkWebGPURenderer* wgpuRenderer;
  vtkSmartPointer<vtkWebGPUComputePipeline> dynamicColorsCompute;
  vtkSmartPointer<vtkWebGPUComputePipeline> dynamicPositionsCompute;

  dynamicColorsCompute = SetupPointColorsPipeline(webGPUMapper, polydata);
  dynamicPositionsCompute = SetupPointPositionsPipeline(webGPUMapper, polydata);
  // Adding the compute pipeline to the renderer.
  // The pipeline will be executed each frame before the rendering pass
  wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renWin->GetRenderers()->GetFirstRenderer());
  wgpuRenderer->AddComputePipeline(dynamicColorsCompute);

  renWin->Render();

  // Only adding the compute pipeline for the positions now because we only want it to execute
  // starting on the second frame
  wgpuRenderer->AddComputePipeline(dynamicPositionsCompute);
  renWin->Render();

  // Screenshot taken by the regression testing isn't flipped.
  // This isn't an issue for testing but that may be something to look into
  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

  return !retVal;
}
