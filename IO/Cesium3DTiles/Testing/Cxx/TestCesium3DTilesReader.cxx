// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This tests reading a file using a PDAL reader.
 */

#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkCesium3DTilesReader.h"
#include "vtkDataArray.h"
#include "vtkGLTFReader.h"
#include "vtkGLTFTexture.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

void AddActors(
  vtkRenderer* renderer, vtkPartitionedDataSetCollection* pdc, vtkCesium3DTilesReader* reader)
{
  for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
  {
    vtkPartitionedDataSet* pd = pdc->GetPartitionedDataSet(i);
    auto gltfReader = reader->GetTileReader(i);
    for (unsigned int j = 0; j < pd->GetNumberOfPartitions(); ++j)
    {
      vtkPolyData* poly = vtkPolyData::SafeDownCast(pd->GetPartition(j));
      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInputDataObject(poly);

      vtkNew<vtkActor> actor;
      actor->SetMapper(mapper);
      renderer->AddActor(actor);
      auto t = gltfReader->GetTexture(j);
      auto texture = t->GetVTKTexture();
      // flip texture coordinates
      if (actor->GetPropertyKeys() == nullptr)
      {
        vtkNew<vtkInformation> info;
        actor->SetPropertyKeys(info);
      }
      double mat[] = { 1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
      actor->GetPropertyKeys()->Set(vtkProp::GeneralTextureTransform(), mat, 16);

      actor->SetTexture(texture);
    }
  }
}

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

  vtkSmartPointer<vtkPartitionedDataSetCollection> outputData = reader->GetOutput();

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.7, 0.7);
  AddActors(renderer, outputData, reader);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->Azimuth(-45);
  renderer->GetActiveCamera()->Zoom(1.2);

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
