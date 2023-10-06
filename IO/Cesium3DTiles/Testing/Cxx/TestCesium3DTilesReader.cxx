// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This tests reading a file using a PDAL reader.
 */

#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkCesium3DTilesReader.h"
#include "vtkDataArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

int TestCesium3DTilesReader(int argc, char* argv[])
{
  const char* fileName = "Data/3DTiles/jacksonville-gltf/tileset.json";
  const char* path = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName);
  vtkNew<vtkCesium3DTilesReader> reader;
  // Select source file
  reader->SetFileName(path);
  delete[] path;

  // Read the output
  reader->Update();

  vtkSmartPointer<vtkPartitionedDataSet> outputData = reader->GetOutput();

  vtkNew<vtkAppendPolyData> append;
  for (unsigned int i = 0; i < outputData->GetNumberOfPartitions(); ++i)
    append->AddInputDataObject(0, outputData->GetPartition(i));
  append->Update();

  // Visualise in a render window
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(vtkPolyData::SafeDownCast(append->GetOutputDataObject(0)));

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Elevation(-90);

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.2);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
