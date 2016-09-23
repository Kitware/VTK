/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkNew.h"
#include "vtkProperty.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

#include "vtkOpenGLRenderWindow.h"

#include "vtkOpenVRCamera.h"
#include "vtkCullerCollection.h"
#include "vtkTransform.h"

#include "vtkPlaneWidget.h"

#include "vtkTransformPolyDataFilter.h"

#include "vtkLight.h"

#include "vtkOpenVRRenderer.h"
     #include "vtkOpenVRCamera.h"
     #include "vtkOpenVRRenderWindow.h"
     #include "vtkOpenVRRenderWindowINteractor.h"

// #include "vtkLODPointCloudMapper.h"
// #include "vtkXMLMultiBlockDataReader.h"

//----------------------------------------------------------------------------
int TestDragon(int argc, char *argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkOpenVRRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);
  vtkNew<vtkOpenVRRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkOpenVRRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());
  vtkNew<vtkOpenVRCamera> cam;
  renderer->SetActiveCamera(cam.Get());

  // crazy frame rate requirement
  // need to look into that at some point
  renderWindow->SetDesiredUpdateRate(350.0);
  iren->SetDesiredUpdateRate(350.0);
  iren->SetStillUpdateRate(350.0);

  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  vtkNew<vtkLight> light;
  light->SetLightTypeToCameraLight();
  light->SetPosition(1.0, 1.0, 1.0);
  renderer->AddLight(light.Get());


  // vtkNew<vtkXMLMultiBlockDataReader> reader;
  // reader->SetFileName("C:/Users/Kenny/Documents/vtk/village.vtmopc");
  // reader->SetFileName("C:/Users/Kenny/Documents/vtk/20M.vtmopc");
  // reader->Update();
  // vtkNew<vtkLODPointCloudMapper> mapper;
  // mapper->SetInputConnection(reader->GetOutputPort(0));
  // actor->SetMapper(mapper.Get());

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  vtkNew<vtkTransform> trans;
  trans->Translate(10.0,20.0,30.0);
  vtkNew<vtkTransformPolyDataFilter> tf;
  tf->SetTransform(trans.Get());
  tf->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tf->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
//  actor->GetProperty()->SetRepresentationToWireframe();

  // vtkNew<vtkPlaneWidget> planeWidget;
  // planeWidget->SetInteractor(iren.Get());
  // planeWidget->SetInputData(tf->GetOutput());
  // planeWidget->PlaceWidget(actor->GetBounds());
  // planeWidget->SetRepresentationToWireframe();
  // planeWidget->SetEnabled(1);

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
