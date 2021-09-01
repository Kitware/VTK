/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSApplyDisplacementsCGNS.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkCamera.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositeDataSetRange.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataArraySelection.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
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

int TestIOSSApplyDisplacementsCGNS(int argc, char* argv[])
{
  auto fname = GetFileName(
    argc, argv, std::string("Data/CGNS/r24_node_tailor_test_output/sparc-volume.cgns.36.00"));
  vtkNew<vtkIOSSReader> reader;
  reader->AddFileName(fname.c_str());
  reader->GenerateFileIdOn();
  reader->UpdateInformation();
  reader->GetSideSetSelection()->EnableAllArrays();
  reader->UpdateTimeStep(0.000183259); // last timestep.

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputDataObject(reader->GetOutputDataObject(0));
  mapper->SetInputConnection(surface->GetOutputPort());
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
