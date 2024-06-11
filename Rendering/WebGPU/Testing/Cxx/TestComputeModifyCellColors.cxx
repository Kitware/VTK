// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test ensures that a compute pipeline is able to modify the existing render data of a render
 * pipeline.
 * This is tested by turning white the color of the cell of a triangle using a compute pipeline.
 */

#include "TestComputeModifyCellColorsShader.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

int TestComputeModifyCellColors(int argc, char* argv[])
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
  colors->SetNumberOfTuples(1);
  colors->InsertComponent(0, 0, 255);
  colors->InsertComponent(0, 1, 0);
  colors->InsertComponent(0, 2, 0);
  colors->InsertComponent(0, 3, 255);
  polydata->GetCellData()->SetScalars(colors);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Getting the WebGPUMapper to access the cell attribute render buffers
  vtkWebGPUPolyDataMapper* webGPUMapper = vtkWebGPUPolyDataMapper::SafeDownCast(mapper);

  // Getting the cell colors render buffer and indicating that we want it to be bound to (0,
  // 0) and the uniform buffer to be bound to (0, 1)
  int bufferGroup = 0;
  int bufferBinding = 0;
  int uniformsGroup = 0;
  int uniformsBinding = 1;
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> cellColorsRenderBuffer =
    webGPUMapper->AcquireCellAttributeComputeRenderBuffer(
      vtkWebGPUPolyDataMapper::CellDataAttributes::CELL_COLORS, bufferGroup, bufferBinding,
      uniformsGroup, uniformsBinding);
  // Label in case something goes wrong
  cellColorsRenderBuffer->SetLabel("Cell colors render buffer");

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> cellColorCompute;
  cellColorCompute->SetShaderSource(TestComputeModifyCellColorsShader);
  cellColorCompute->SetShaderEntryPoint("changeCellColorCompute");
  // Adding the render buffer
  cellColorCompute->AddRenderBuffer(cellColorsRenderBuffer);
  int nbGroupsX = std::ceil(polydata->GetCellData()->GetScalars()->GetNumberOfTuples() / 32.0f);
  cellColorCompute->SetWorkgroups(nbGroupsX, 1, 1);

  // Adding the compute pipeline to the renderer.
  // The pipeline will be executed each frame before the rendering pass
  vtkWebGPURenderer* wegpuRenderer =
    vtkWebGPURenderer::SafeDownCast(renWin->GetRenderers()->GetFirstRenderer());
  wegpuRenderer->AddComputePipeline(cellColorCompute);

  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();

  // Screenshot taken by the regression testing isn't flipped.
  // This isn't an issue for testing but that may be something to look into
  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

  return !retVal;
}
