/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSTri6.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test for paraview/paraview#19404
 */
#include <vtkCamera.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkDataArraySelection.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
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

int TestIOSSTri6(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  reader->SetFileName(
    ::GetFileName(argc, argv, "Data/Exodus/SAND2020-4077_O-tri6sWFace2.exo").c_str());
  reader->UpdateInformation();

  // hide blocks and enable sets.
  for (int cc = vtkIOSSReader::ENTITY_START; cc < vtkIOSSReader::ENTITY_END; ++cc)
  {
    auto sel = reader->GetEntitySelection(cc);
    if (vtkIOSSReader::GetEntityTypeIsBlock(cc))
    {
      sel->DisableAllArrays();
    }
    else if (vtkIOSSReader::GetEntityTypeIsSet(cc))
    {
      sel->EnableAllArrays();
    }
  }

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
