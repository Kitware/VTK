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
#include "vtkImageGridSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkSkybox.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include "vtkLight.h"

//----------------------------------------------------------------------------
int TestFloor(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 7.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkImageGridSource> grid;
  grid->SetGridSpacing(32, 32, 0);

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToMapScalars();
  vtkNew<vtkLookupTable> lut;
  texture->SetLookupTable(lut);
  lut->SetSaturationRange(0.0, 0.0);
  lut->SetValueRange(0.0, 1.0);
  lut->SetTableRange(0.0, 1.0);
  lut->Build();
  texture->InterpolateOn();
  texture->RepeatOn();
  texture->MipmapOn();
  texture->SetInputConnection(grid->GetOutputPort(0));

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetPosition(0, 0, 0);
  actor->SetScale(6.0, 6.0, 6.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetSpecularPower(20);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.4);
  actor->GetProperty()->SetAmbientColor(0.4, 0.0, 1.0);
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  vtkNew<vtkSkybox> floor;
  floor->SetProjectionToFloor();
  floor->SetTexture(texture);
  renderer->AddActor(floor);

  renderer->GetActiveCamera()->SetPosition(0.0, 0.55, 2.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.55, 0.0);
  renderer->GetActiveCamera()->SetViewAngle(60.0);
  renderer->GetActiveCamera()->Zoom(1.1);
  renderer->GetActiveCamera()->Azimuth(0);
  renderer->GetActiveCamera()->Elevation(5);
  renderer->GetActiveCamera()->Roll(-10);
  renderer->SetBackground(0.6, 0.7, 1.0);
  renderer->ResetCameraClippingRange();

  renderWindow->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style);

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
