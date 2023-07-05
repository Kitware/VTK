// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Reads a partitioned exodus file in parallel
 */
#include <vtkArrayCalculator.h>
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataObject.h>
#include <vtkLogger.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkXMLMultiBlockDataReader.h>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);

  vtkLogF(INFO, "filename: %s", fileNameC);
  delete[] fileNameC;
  return fname;
}

vtkSmartPointer<vtkActor> GetActor(
  int argc, char* argv[], const std::string& suffix, int numPieces, int mode)
{
  vtkNew<vtkMultiBlockDataSet> mb;
  for (int idx = 0; idx < numPieces; ++idx)
  {
    vtkNew<vtkXMLMultiBlockDataReader> reader;
    reader->SetFileName(::GetFileName(argc, argv, suffix).c_str());
    reader->SetPieceDistribution(mode);

    vtkNew<vtkArrayCalculator> calculator;
    calculator->SetFunction(std::to_string(idx).c_str());
    calculator->SetResultArrayName("piece-id");
    calculator->SetAttributeTypeToCellData();
    calculator->SetInputConnection(reader->GetOutputPort());
    calculator->UpdatePiece(idx, numPieces, 0);
    mb->SetBlock(idx, calculator->GetOutputDataObject(0));
  }

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(mb);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetColorModeToMapScalars();
  mapper->SelectColorArray("piece-id");
  mapper->SetScalarRange(0, numPieces - 1);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  return actor;
}

int TestXMLPieceDistribution(int argc, char* argv[])
{
  const int numPieces = 3;

  // These are multiblocks that have multipieces alone. These should be
  // split with each piece distributed across the ranks separately.
  auto actor0 =
    GetActor(argc, argv, "Data/mb-of-mps.vtm", numPieces, vtkXMLCompositeDataReader::Block);
  auto actor1 =
    GetActor(argc, argv, "Data/mb-of-mps.vtm", numPieces, vtkXMLCompositeDataReader::Interleave);
  actor1->SetPosition(0, 3.5, 0);

  // These are multipieces that have multipieces and others. These are
  // distributed using the legacy mechanism where all leaves are treated
  // as a whole and evenly distributed.
  auto actor2 =
    GetActor(argc, argv, "Data/mixed-mb.vtm", numPieces, vtkXMLCompositeDataReader::Block);
  actor2->SetPosition(0, 7.0, 0);
  actor2->GetProperty()->EdgeVisibilityOn();
  auto actor3 =
    GetActor(argc, argv, "Data/mixed-mb.vtm", numPieces, vtkXMLCompositeDataReader::Interleave);
  actor3->SetPosition(0, 10.5, 0);
  actor3->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 600);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->AddActor(actor0);
  ren->AddActor(actor1);
  ren->AddActor(actor2);
  ren->AddActor(actor3);

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(2.0);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren = nullptr;
  return retVal != vtkTesting::FAILED ? EXIT_SUCCESS : EXIT_FAILURE;
}
