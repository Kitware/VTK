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
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLVertexBufferObject.h"
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

#include "vtkWin32RenderWindowInteractor.h"
#include "vtkWin32OpenGLRenderWindow.h"

//----------------------------------------------------------------------------
int TestDragon(int argc, char *argv[])
{
  vtkNew<vtkOpenVRRenderer> renderer;
  vtkNew<vtkOpenVRRenderWindow> renderWindow;
  vtkNew<vtkOpenVRRenderWindowInteractor>  iren;
  vtkNew<vtkOpenVRCamera> cam;
  renderer->SetShowFloor(true);

  vtkNew<vtkActor> actor;
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  iren->SetRenderWindow(renderWindow.Get());
  renderer->SetActiveCamera(cam.Get());

  //renderer->UseShadowsOn();

  // crazy frame rate requirement
  // need to look into that at some point
  renderWindow->SetDesiredUpdateRate(350.0);
  iren->SetDesiredUpdateRate(350.0);
  iren->SetStillUpdateRate(350.0);

  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 1.0, 1.0);
  renderer->AddLight(light.Get());

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  vtkNew<vtkTransform> trans;
  trans->Translate(10.0,20.0,30.0);
  //trans->Scale(10.0,10.0,10.0);
  vtkNew<vtkTransformPolyDataFilter> tf;
  tf->SetTransform(trans.Get());
  tf->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(tf->GetOutputPort());
  mapper->SetVBOShiftScaleMethod(vtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE);
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

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
