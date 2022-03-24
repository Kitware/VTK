/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAnimateModes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnimateModes.h"
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataArraySelection.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
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

int TestAnimateModes(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.0"));
  reader->AddFileName(fname.c_str());
  reader->ApplyDisplacementsOn();
  reader->UpdateInformation();
  reader->GetNodeBlockFieldSelection()->EnableAllArrays();

  vtkNew<vtkAnimateModes> modeShapes;
  modeShapes->SetInputConnection(reader->GetOutputPort());
  modeShapes->UpdateInformation();

  const auto mrange = vtkVector2i(modeShapes->GetModeShapesRange());
  if (mrange != vtkVector2i(1, 44))
  {
    vtkLogF(ERROR,
      "Invalid mode-shape range, expected [1, 44], "
      "got [%d, %d]",
      mrange[0], mrange[1]);
    return EXIT_FAILURE;
  }
  modeShapes->SetModeShape(11);
  modeShapes->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DISPL");
  modeShapes->DisplacementPreappliedOn();
  modeShapes->SetDisplacementMagnitude(2.0);
  modeShapes->AnimateVibrationsOn();
  modeShapes->UpdateInformation();
  modeShapes->UpdateTimeStep(0.5);

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputDataObject(modeShapes->GetOutputDataObject(0));
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
