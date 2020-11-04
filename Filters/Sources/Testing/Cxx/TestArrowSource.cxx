/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArrowSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkExecutive.h>
#include <vtkExtractGrid.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiBlockPLOT3DReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

int TestArrowSource(int argc, char* argv[])
{
  vtkSmartPointer<vtkArrowSource> arrow = vtkSmartPointer<vtkArrowSource>::New();
  vtkSmartPointer<vtkArrowSource> arrowCentral = vtkSmartPointer<vtkArrowSource>::New();
  vtkSmartPointer<vtkArrowSource> arrowInvertCentral = vtkSmartPointer<vtkArrowSource>::New();

  double shaftRadius = arrow->GetShaftRadius();
  arrow->SetShaftRadius(shaftRadius * 2.0);
  arrowCentral->SetShaftRadius(shaftRadius * 2.0);
  arrowInvertCentral->SetShaftRadius(shaftRadius * 2.0);

  double shaftRes = arrow->GetShaftResolution();
  arrow->SetShaftResolution(shaftRes * 15.0);
  arrowCentral->SetShaftResolution(shaftRes * 15.0);
  arrowInvertCentral->SetShaftResolution(shaftRes * 15.0);

  double tipRes = arrow->GetTipResolution();
  arrow->SetTipResolution(tipRes * 10.0);
  arrowCentral->SetTipResolution(tipRes * 10.0);
  arrowInvertCentral->SetTipResolution(tipRes * 10.0);

  // Centralize just these arrows
  arrowCentral->SetArrowOriginToCenter();
  arrowInvertCentral->SetArrowOriginToCenter();

  // Invert only this arrow
  arrowInvertCentral->InvertOn();

  arrow->Update();
  arrowCentral->Update();
  arrowInvertCentral->Update();

  vtkSmartPointer<vtkPolyData> polydata = arrow->GetOutput();
  vtkSmartPointer<vtkPolyData> polydataCentral = arrowCentral->GetOutput();
  vtkSmartPointer<vtkPolyData> polydataInvertCentral = arrowInvertCentral->GetOutput();

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapperCentral = vtkSmartPointer<vtkPolyDataMapper>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapperInvertCentral =
    vtkSmartPointer<vtkPolyDataMapper>::New();

  mapper->SetInputData(polydata);
  mapper->ScalarVisibilityOff();

  mapperCentral->SetInputData(polydataCentral);
  mapperCentral->ScalarVisibilityOff();

  mapperInvertCentral->SetInputData(polydataInvertCentral);
  mapperInvertCentral->ScalarVisibilityOff();

  // Normal Arrow
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->SetPosition(0.0, 0.325, 0.0);

  vtkProperty* propertyToSet = actor->GetProperty();
  propertyToSet->SetDiffuseColor(0.501, 1.0, 0.0);
  propertyToSet->SetSpecular(0.15);
  propertyToSet->SetSpecularPower(5.0);

  // Centralized Arrow
  vtkActor* actorCentral = vtkActor::New();
  actorCentral->SetMapper(mapperCentral);
  actorCentral->SetPosition(0.0, 0.0, 0.0);

  vtkProperty* propertyToSetCentral = actorCentral->GetProperty();
  propertyToSetCentral->SetDiffuseColor(1.0, 0.647, 0.0);
  propertyToSetCentral->SetSpecular(0.15);
  propertyToSetCentral->SetSpecularPower(5.0);

  // Inverted - Centralized Arrow
  vtkActor* actorInvertCentral = vtkActor::New();
  actorInvertCentral->SetMapper(mapperInvertCentral);
  actorInvertCentral->SetPosition(0.0, -0.325, 0.0);

  vtkProperty* propertyToSetInvertCentral = actorInvertCentral->GetProperty();
  propertyToSetInvertCentral->SetDiffuseColor(0.2, 0.8, 1.0);
  propertyToSetInvertCentral->SetSpecular(0.25);
  propertyToSetInvertCentral->SetSpecularPower(5.0);

  // Setup renderer.
  vtkRenderer* ren = vtkRenderer::New();
  vtkRenderWindow* win = vtkRenderWindow::New();
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  ren->Delete();
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(win);
  win->Delete();

  // Add all arrow actors for display.
  ren->AddActor(actor);
  ren->AddActor(actorCentral);
  ren->AddActor(actorInvertCentral);

  // cleanup.
  actor->Delete();
  actorCentral->Delete();
  actorInvertCentral->Delete();

  ren->SetBackground(0.0, 0.0, 0.0);
  win->SetSize(450, 450);

  vtkCamera* cam = ren->GetActiveCamera();
  cam->SetPosition(-2.3332, 1.0, 2.25);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
