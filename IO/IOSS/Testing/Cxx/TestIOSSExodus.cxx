/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSExodus.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
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

int TestIOSSExodus(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.0"));
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

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
