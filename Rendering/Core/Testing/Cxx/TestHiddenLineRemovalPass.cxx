/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkExodusIIReader.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"

int TestHiddenLineRemovalPass(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin.Get());
  vtkNew<vtkRenderer> renderer;
  renderer->UseHiddenLineRemovalOn();
  renWin->AddRenderer(renderer.Get());

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  vtkNew<vtkExodusIIReader> reader;
  reader->SetFileName(fileName);
  delete [] fileName;

  vtkNew<vtkCompositeDataGeometryFilter> geomFilter;
  geomFilter->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(geomFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetColor(1., 0., 0.);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor.Get());

  // Workaround a rendering bug. See gitlab issue #16816.
  actor->GetProperty()->LightingOff();

  renWin->SetSize(500,500);
  renderer->SetBackground(8., 7., 1.);
  renderer->SetBackground2(.3, .1, .2);
  renderer->GradientBackgroundOn();
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->SetPosition(-340., -70., -50.);
  renderer->GetActiveCamera()->SetFocalPoint(-2.5, 3., -5.);
  renderer->GetActiveCamera()->SetViewUp(0, 0.5, -1);
  renderer->GetActiveCamera()->SetParallelScale(12);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin.Get());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
