/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestShadowMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the shadow map render pass.
// The scene consists of
// * 4 actors: a rectangle, a box, a cone and a sphere. The box, the cone and
// the sphere are above the rectangle.
// * 2 spotlights: one in the direction of the box, another one in the
// direction of the sphere. Both lights are above the box, the cone and
// the sphere.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
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
#include "vtkShadowMapBakerPass.h"
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
#include "vtkPolyDataNormals.h"
#include "vtkPointData.h"

// Defined in TestLightActor.cxx
// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer *r);

int TestShadowMapPass(int argc, char* argv[])
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

  vtkOpenGLRenderer *glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);

  vtkCameraPass *cameraP=vtkCameraPass::New();

  vtkOpaquePass *opaque=vtkOpaquePass::New();

  vtkDepthPeelingPass *peeling=vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkTranslucentPass *translucent=vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);

  vtkVolumetricPass *volume=vtkVolumetricPass::New();
  vtkOverlayPass *overlay=vtkOverlayPass::New();

  vtkLightsPass *lights=vtkLightsPass::New();

  vtkSequencePass *opaqueSequence=vtkSequencePass::New();

  vtkRenderPassCollection *passes2=vtkRenderPassCollection::New();
  passes2->AddItem(lights);
  passes2->AddItem(opaque);
  opaqueSequence->SetPasses(passes2);

  vtkCameraPass *opaqueCameraPass=vtkCameraPass::New();
  opaqueCameraPass->SetDelegatePass(opaqueSequence);

  vtkShadowMapBakerPass *shadowsBaker=vtkShadowMapBakerPass::New();
  shadowsBaker->SetOpaquePass(opaqueCameraPass);
  shadowsBaker->SetResolution(1024);
  // To cancel self-shadowing.
  shadowsBaker->SetPolygonOffsetFactor(3.1f);
  shadowsBaker->SetPolygonOffsetUnits(10.0f);

  vtkShadowMapPass *shadows=vtkShadowMapPass::New();
  shadows->SetShadowMapBakerPass(shadowsBaker);
  shadows->SetOpaquePass(opaqueSequence);

  vtkSequencePass *seq=vtkSequencePass::New();
  vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
  passes->AddItem(shadowsBaker);
  passes->AddItem(shadows);
  passes->AddItem(lights);
  passes->AddItem(peeling);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);


  glrenderer->SetPass(cameraP);

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
  rectangleKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(),0); // dummy val.
  rectangleKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(),0); // dummy val.
  rectangleActor->SetPropertyKeys(rectangleKeyProperties);
  rectangleKeyProperties->Delete();
  rectangleActor->SetMapper(rectangleMapper);
  rectangleMapper->Delete();
  rectangleActor->SetVisibility(1);
  rectangleActor->GetProperty()->SetColor(1.0,1.0,1.0);

  vtkCubeSource *boxSource=vtkCubeSource::New();
  boxSource->SetXLength(2.0);
  vtkPolyDataNormals *boxNormals=vtkPolyDataNormals::New();
  boxNormals->SetInputConnection(boxSource->GetOutputPort());
  boxNormals->SetComputePointNormals(0);
  boxNormals->SetComputeCellNormals(1);
  boxNormals->Update();
  boxNormals->GetOutput()->GetPointData()->SetNormals(0);

  vtkPolyDataMapper *boxMapper=vtkPolyDataMapper::New();
  boxMapper->SetInputConnection(boxNormals->GetOutputPort());
  boxNormals->Delete();
  boxSource->Delete();
  boxMapper->SetScalarVisibility(0);

  vtkActor *boxActor=vtkActor::New();
  vtkInformation *boxKeyProperties=vtkInformation::New();
  boxKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(),0); // dummy val.
  boxKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(),0); // dummy val.
  boxActor->SetPropertyKeys(boxKeyProperties);
  boxKeyProperties->Delete();

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
  vtkInformation *coneKeyProperties=vtkInformation::New();
  coneKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(),0); // dummy val.
  coneKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(),0); // dummy val.
  coneActor->SetPropertyKeys(coneKeyProperties);
  coneKeyProperties->Delete();
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
  vtkInformation *sphereKeyProperties=vtkInformation::New();
  sphereKeyProperties->Set(vtkShadowMapBakerPass::OCCLUDER(),0); // dummy val.
  sphereKeyProperties->Set(vtkShadowMapBakerPass::RECEIVER(),0); // dummy val.
  sphereActor->SetPropertyKeys(sphereKeyProperties);
  sphereKeyProperties->Delete();
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
  l1->SetSwitch(1);
  l1->Delete();

  // lighting the sphere
  vtkLight *l2=vtkLight::New();
  l2->SetPosition(4.0,5.0,1.0);
  l2->SetFocalPoint(sphereActor->GetPosition());
  l2->SetColor(1.0,0.0,1.0);
//  l2->SetColor(1.0,1.0,1.0);
  l2->SetPositional(1);
  renderer->AddLight(l2);
  l2->SetSwitch(1);
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

   opaqueCameraPass->Delete();
  opaqueSequence->Delete();
  passes2->Delete();
  shadows->Delete();
  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  shadowsBaker->Delete();

  return !retVal;
}
