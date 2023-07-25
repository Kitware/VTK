// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * This test tests that vtkIOSSWriter can detect and create restarts when input
 * mesh changes and cell types are not preserved.
 */
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataArraySelection.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkIOSSWriter.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

namespace
{
std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

std::string GetOutputFileName(int argc, char* argv[], const std::string& suffix)
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  auto* tempDir = testing->GetTempDirectory();
  if (!tempDir)
  {
    vtkLogF(ERROR, "No output directory specified!");
    return {};
  }

  return std::string(tempDir) + "/" + suffix;
}
}

int TestIOSSExodusWriterClip(int argc, char* argv[])
{
  auto ofname = GetOutputFileName(argc, argv, "test_ioss_exodus_writer_clip.ex2");
  if (ofname.empty())
  {
    return EXIT_FAILURE;
  }

  // Write data
  vtkNew<vtkIOSSReader> reader0;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.0"));
  reader0->SetFileName(fname.c_str());
  reader0->SetGroupNumericVectorFieldComponents(true);
  reader0->UpdateInformation();
  reader0->GetElementBlockSelection()->EnableAllArrays();
  reader0->GetNodeSetSelection()->EnableAllArrays();
  reader0->GetSideSetSelection()->EnableAllArrays();

  vtkNew<vtkPlane> plane;
  plane->SetNormal(1, 0, 0);
  plane->SetOrigin(0.21706008911132812, 4, -5.110947132110596);

  vtkNew<vtkTableBasedClipDataSet> clipper;
  clipper->SetClipFunction(plane);
  clipper->SetInputConnection(reader0->GetOutputPort());

  vtkNew<vtkIOSSWriter> writer;
  writer->SetFileName(ofname.c_str());
  writer->SetInputConnection(clipper->GetOutputPort());
  writer->Write();

  // Open the saved file and render it.
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName(ofname.c_str());
  reader->SetGroupNumericVectorFieldComponents(true);
  reader->GetElementBlockSelection()->EnableAllArrays();
  reader->GetNodeSetSelection()->EnableAllArrays();
  reader->GetSideSetSelection()->EnableAllArrays();
  reader->UpdateInformation();
  reader->UpdateTimeStep(0.00100001);

  const std::vector<std::string> elementBlocks{ "block_1", "block_2", "block_1_tetra4",
    "block_1_hex8", "block_1_wedge6", "block_1_pyramid5", "block_2_tetra4", "block_2_hex8",
    "block_2_wedge6", "block_2_pyramid5" };
  const auto elementBlockSelection = reader->GetElementBlockSelection();
  for (int i = 0; i < elementBlockSelection->GetNumberOfArrays(); ++i)
  {
    if (elementBlockSelection->GetArrayName(i) != elementBlocks[i])
    {
      vtkLogF(ERROR, "Element block %d is not %s", i, elementBlocks[i].c_str());
      return EXIT_FAILURE;
    }
  }

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputDataObject(reader->GetOutputDataObject(0));
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

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
