/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSExodusWriterCrinkleClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * This test tests that vtkIOSSWriter can detect and create restarts when input
 * mesh changes.
 */
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataArraySelection.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkExtractGeometry.h>
#include <vtkIOSSReader.h>
#include <vtkIOSSWriter.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

static std::string GetOutputFileName(int argc, char* argv[], const std::string& suffix)
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

int TestIOSSExodusWriterCrinkleClip(int argc, char* argv[])
{
  auto ofname = GetOutputFileName(argc, argv, "test_ioss_exodus_writer_crinkle_clip.ex2");
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
  plane->SetNormal(1, 1, 1);

  vtkNew<vtkExtractGeometry> clipper;
  clipper->SetImplicitFunction(plane);
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
