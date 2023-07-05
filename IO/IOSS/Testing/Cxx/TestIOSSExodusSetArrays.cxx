// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCamera.h>
#include <vtkClipDataSet.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataArraySelection.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestIOSSExodusSetArrays(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/biplane_rms_pressure_bs.exo"));
  reader->AddFileName(fname.c_str());
  reader->UpdateInformation();
  reader->GetElementBlockSelection()->DisableAllArrays();
  reader->GetSideSetSelection()->EnableArray("surface_10");
  reader->GetSideSetFieldSelection()->EnableAllArrays();

  // Applying clip to test paraview/paraview#21342
  vtkNew<vtkClipDataSet> clipper;
  clipper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPlane> plane;
  plane->SetNormal(1, 0, 0);
  plane->SetOrigin(0, 0, 0);
  clipper->SetClipFunction(plane);

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputConnection(clipper->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("PressureRMS");
  mapper->ScalarVisibilityOn();
  mapper->UseLookupTableScalarRangeOff();
  mapper->SetScalarRange(0, 1);
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();
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
