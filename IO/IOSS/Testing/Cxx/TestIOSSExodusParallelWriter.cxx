// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Reads a partitioned exodus file in parallel, clips it, writes it out, and reads it back
 */
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkCompositedSynchronizedRenderers.h>
#include <vtkDataArraySelection.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkGenerateProcessIds.h>
#include <vtkIOSSReader.h>
#include <vtkIOSSWriter.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPlane.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTableBasedClipDataSet.h>
#include <vtkTestUtilities.h>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include <vtkMPIController.h>
#include <vtkSynchronizedRenderWindows.h>
#else
#include "vtkDummyController.h"
#endif

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

int TestIOSSExodusParallelWriter(int argc, char* argv[])
{
  auto ofname = GetOutputFileName(argc, argv, "test_ioss_exodus_parallel_writer.ex2");
  if (ofname.empty())
  {
    return EXIT_FAILURE;
  }
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  const int myId = contr->GetLocalProcessId();
  const int numProcs = contr->GetNumberOfProcesses();

  vtkNew<vtkIOSSReader> reader0;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.0"));
  reader0->SetFileName(fname.c_str());
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
  writer->PreserveOriginalIdsOn();
  writer->Write();

  vtkNew<vtkIOSSReader> reader;
  reader->ReadAllFilesToDetermineStructureOn();
  if (contr->GetNumberOfProcesses() == 1)
  {
    reader->SetFileName(ofname.c_str());
  }
  else
  {
    auto parallelOutputFileName =
      ofname + "." + std::to_string(contr->GetNumberOfProcesses()) + ".0";
    reader->SetFileName(parallelOutputFileName.c_str());
  }
  reader->UpdateInformation();
  reader->GetElementBlockSelection()->EnableAllArrays();
  reader->GetNodeSetSelection()->EnableAllArrays();
  reader->GetSideSetSelection()->EnableAllArrays();

  vtkNew<vtkGenerateProcessIds> pidGenerator;
  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;

  pidGenerator->SetInputConnection(reader->GetOutputPort());
  pidGenerator->GeneratePointDataOff();
  pidGenerator->GenerateCellDataOn();
  pidGenerator->Update();
  vtkPartitionedDataSetCollection* pidOutputCollection =
    vtkPartitionedDataSetCollection::SafeDownCast(pidGenerator->GetOutputDataObject(0));
  vtkDataSet* pidOutput = pidOutputCollection->GetPartition(0, 0);

  surface->SetInputConnection(pidGenerator->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetPiece(myId);
  mapper->SetNumberOfPieces(numProcs);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetColorModeToMapScalars();
  mapper->SelectColorArray(pidOutput->GetCellData()->GetProcessIds()->GetName());
  mapper->SetScalarRange(0, numProcs - 1);

  // update mapper an get parallel bounds.
  mapper->Update();
  vtkBoundingBox bbox(mapper->GetBounds());
  contr->AllReduce(bbox, bbox);
  double bds[6];
  bbox.GetBounds(bds);

  actor->SetMapper(mapper);
  renWin->AddRenderer(ren);

  ren->AddActor(actor);
  renWin->SetSize(300, 300);
  auto cam = ren->GetActiveCamera();
  cam->SetPosition(10., 10., 5.);
  cam->SetViewUp(0., 0.4, 1.);
  ren->ResetCamera(bds);
  ren->ResetCameraClippingRange(bds);

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkSynchronizedRenderWindows> syncWindows;
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(contr);
  syncWindows->SetIdentifier(1);
#endif

  vtkNew<vtkCompositedSynchronizedRenderers> syncRenderers;
  syncRenderers->SetRenderer(ren);
  syncRenderers->SetParallelController(contr);

  int retVal = EXIT_FAILURE;
  if (myId == 0)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
    iren->Initialize();
    retVal = vtkRegressionTestImage(renWin);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    contr->TriggerBreakRMIs();
    contr->Broadcast(&retVal, 1, 0);
  }
  else
  {
    renWin->OffScreenRenderingOn();
    contr->ProcessRMIs();
    contr->Broadcast(&retVal, 1, 0);
  }
  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return !retVal;
}
