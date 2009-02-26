/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLightActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the vtkLightActor and vtkCameraActor for scene
// introspection.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"

#include "vtkImageSinusoidSource.h"
#include "vtkImageData.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkCamera.h"

#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkSequencePass.h"
#include "vtkOpaquePass.h"
#include "vtkDepthPeelingPass.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkShadowMapPass.h"
#include "vtkConeSource.h"
#include "vtkPlaneSource.h"
#include "vtkCubeSource.h"
#include "vtkSphereSource.h"
#include "vtkInformation.h"
#include "vtkProperty.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include <assert.h>
#include "vtkMath.h"
#include "vtkFrustumSource.h"
#include "vtkPlanes.h"
#include "vtkActorCollection.h"
#include "vtkLightActor.h"
#include "vtkCameraActor.h"

// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer *r);

int TestLightActor(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  
  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  vtkCameraPass *cameraP=vtkCameraPass::New();
  
  vtkSequencePass *seq=vtkSequencePass::New();
  vtkOpaquePass *opaque=vtkOpaquePass::New();
  vtkDepthPeelingPass *peeling=vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);
  
  vtkTranslucentPass *translucent=vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);
  
  vtkVolumetricPass *volume=vtkVolumetricPass::New();
  vtkOverlayPass *overlay=vtkOverlayPass::New();
  
  vtkLightsPass *lights=vtkLightsPass::New();
  
  vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  
  passes->AddItem(peeling);
//  passes->AddItem(translucent);
  
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);
  
  renderer->SetPass(cameraP);
  
  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  
  // The scene consists of
  // * 4 actors: a rectangle, a box, a cone and a sphere. The box, the cone and
  // the sphere are above the rectangle.
  // * 2 spotlights: one in the direction of the box, another one in the
  // direction of the sphere. Both lights are above the box, the cone and
  // the sphere.
  
  vtkPlaneSource *rectangleSource=vtkPlaneSource::New();
  rectangleSource->SetOrigin(-5.0,0.0,5.0);
  rectangleSource->SetPoint1(5.0,0.0,5.0);
  rectangleSource->SetPoint2(-5.0,0.0,-5.0);
  rectangleSource->SetResolution(100,100);
  
  vtkPolyDataMapper *rectangleMapper=vtkPolyDataMapper::New();
  rectangleMapper->SetInputConnection(rectangleSource->GetOutputPort());
  rectangleSource->Delete();
  rectangleMapper->SetScalarVisibility(0);
  vtkActor *rectangleActor=vtkActor::New();
  vtkInformation *rectangleKeyProperties=vtkInformation::New();
  rectangleKeyProperties->Set(vtkShadowMapPass::OCCLUDER(),0); // dummy val.
  rectangleActor->SetPropertyKeys(rectangleKeyProperties);
  rectangleKeyProperties->Delete();
  rectangleActor->SetMapper(rectangleMapper);
  rectangleMapper->Delete();
  rectangleActor->SetVisibility(1);
  rectangleActor->GetProperty()->SetColor(1.0,1.0,1.0);
  
  vtkCubeSource *boxSource=vtkCubeSource::New();
  boxSource->SetXLength(2.0);
  vtkPolyDataMapper *boxMapper=vtkPolyDataMapper::New();
  boxMapper->SetInputConnection(boxSource->GetOutputPort());
  boxSource->Delete();
  boxMapper->SetScalarVisibility(0);
  vtkActor *boxActor=vtkActor::New();
  boxActor->SetMapper(boxMapper);
  boxMapper->Delete();
  boxActor->SetVisibility(1);
  boxActor->SetPosition(-2.0,2.0,0.0);
  boxActor->GetProperty()->SetColor(1.0,0.0,0.0);
  
  vtkConeSource *coneSource=vtkConeSource::New();
  coneSource->SetResolution(24);
  coneSource->SetDirection(1.0,1.0,1.0);
  vtkPolyDataMapper *coneMapper=vtkPolyDataMapper::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  coneSource->Delete();
  coneMapper->SetScalarVisibility(0);
  vtkActor *coneActor=vtkActor::New();
  coneActor->SetMapper(coneMapper);
  coneMapper->Delete();
  coneActor->SetVisibility(1);
  coneActor->SetPosition(0.0,1.0,1.0);
  coneActor->GetProperty()->SetColor(0.0,0.0,1.0);
//  coneActor->GetProperty()->SetLighting(false);
  
  vtkSphereSource *sphereSource=vtkSphereSource::New();
  sphereSource->SetThetaResolution(32);
  sphereSource->SetPhiResolution(32);
  vtkPolyDataMapper *sphereMapper=vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereSource->Delete();
  sphereMapper->SetScalarVisibility(0);
  vtkActor *sphereActor=vtkActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereMapper->Delete();
  sphereActor->SetVisibility(1);
  sphereActor->SetPosition(2.0,2.0,-1.0);
  sphereActor->GetProperty()->SetColor(1.0,1.0,0.0);
  
  renderer->AddViewProp(rectangleActor);
  rectangleActor->Delete();
  renderer->AddViewProp(boxActor);
  boxActor->Delete();
  renderer->AddViewProp(coneActor);
  coneActor->Delete();
  renderer->AddViewProp(sphereActor);
  sphereActor->Delete();
  
  
  // Spotlights.
  
  // lighting the box.
  vtkLight *l1=vtkLight::New();
  l1->SetPosition(-4.0,4.0,-1.0);
  l1->SetFocalPoint(boxActor->GetPosition());
  l1->SetColor(1.0,1.0,1.0);
  l1->SetPositional(1);
  renderer->AddLight(l1);
  l1->Delete();
  
  // lighting the sphere
  vtkLight *l2=vtkLight::New();
  l2->SetPosition(4.0,5.0,1.0);
  l2->SetFocalPoint(sphereActor->GetPosition());
  l2->SetColor(1.0,0.0,1.0);
  l2->SetPositional(1);
  renderer->AddLight(l2);
  l2->Delete();
  
  
  AddLightActors(renderer);
  
  renderer->SetBackground(0.66,0.66,0.66);
  renderer->SetBackground2(157.0/255.0*0.66,186/255.0*0.66,192.0/255.0*0.66);
  renderer->SetGradientBackground(true);
  renWin->SetSize(400,400);
  
  renWin->Render();
  if(peeling->GetLastRenderingUsedDepthPeeling())
    {
    cout<<"depth peeling was used"<<endl;
    }
  else
    {
    cout<<"depth peeling was not used (alpha blending instead)"<<endl;
    }
  
  renderer->ResetCamera();
  vtkCamera *camera=renderer->GetActiveCamera();
  camera->Azimuth(40.0);
  camera->Elevation(10.0);
  
  renWin->Render();
  
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  iren->Delete();
  
  return !retVal;
}

// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer *r)
{
  assert("pre: r_exists" && r!=0);
  
  vtkLightCollection *lights=r->GetLights();
  
  lights->InitTraversal();
  vtkLight *l=lights->GetNextItem();
  while(l!=0)
    {
    double angle=l->GetConeAngle();
    if(l->LightTypeIsSceneLight() && l->GetPositional()
       && angle<180.0) // spotlight
      {
      vtkLightActor *la=vtkLightActor::New();
      la->SetLight(l);
      r->AddViewProp(la);
      la->Delete();
      }
    l=lights->GetNextItem();
    }
}
