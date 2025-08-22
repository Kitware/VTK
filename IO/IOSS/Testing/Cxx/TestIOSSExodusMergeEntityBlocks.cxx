// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

int TestIOSSExodusMergeEntityBlocks(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  reader->MergeExodusEntityBlocksOn();
  char* fileNameC =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Exodus/can.e.4/can.e.4.0");
  std::string fname(fileNameC);
  delete[] fileNameC;
  reader->AddFileName(fname.c_str());

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputConnection(reader->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  actor->SetMapper(mapper);
  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  renWin->SetSize(300, 300);
  auto cam = ren->GetActiveCamera();
  cam->SetPosition(10., 10., 5.);
  cam->SetViewUp(0., 0.4, 1.);
  ren->ResetCamera();
  renWin->Render();

  // lets verify id maps are built properly
  auto& elementMap = reader->GetElementBlockIdMap();
  auto& nodeSetMap = reader->GetNodeSetIdMap();
  auto& sideSetMap = reader->GetSideSetIdMap();
  if (elementMap.at("block_1") != 1 || elementMap.at("block_2") != 2 ||
    nodeSetMap.at("nodelist_1") != 1 || nodeSetMap.at("nodelist_100") != 100 ||
    sideSetMap.at("surface_4") != 4)
  {
    vtkLogF(ERROR, "id map mismatch!");
  }

  // lets verify that the number of blocks/points/cells
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  if (!output)
  {
    vtkLogF(ERROR, "Expected a vtkPartitionedDataSetCollection but got a %s",
      reader->GetOutputDataObject(0)->GetClassName());
    return EXIT_FAILURE;
  }
  if (output->GetNumberOfPartitionedDataSets() != 3)
  {
    vtkLogF(
      ERROR, "Expected 3 partitioned dataset but got %d", output->GetNumberOfPartitionedDataSets());
    return EXIT_FAILURE;
  }
  if (output->GetNumberOfPoints() != 10516)
  {
    vtkLogF(ERROR, "Expected 10516 points but got %" VTK_ID_TYPE_PRId, output->GetNumberOfPoints());
    return EXIT_FAILURE;
  }
  if (output->GetNumberOfCells() != 7152)
  {
    vtkLogF(ERROR, "Expected 7152 cells but got %" VTK_ID_TYPE_PRId, output->GetNumberOfCells());
    return EXIT_FAILURE;
  }

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
