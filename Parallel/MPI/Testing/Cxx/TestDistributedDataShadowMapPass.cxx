/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistributedDataShadowMapPass.cxx

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

#include "vtkObjectFactory.h"
#include <mpi.h>
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkCompositeRenderManager.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkNew.h"

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
#include "vtkCompositeZPass.h"
#include "vtkConeSource.h"
#include "vtkPlaneSource.h"
#include "vtkCubeSource.h"
#include "vtkSphereSource.h"
#include "vtkInformation.h"
#include "vtkProperty.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include <cassert>
#include "vtkMath.h"
#include "vtkFrustumSource.h"
#include "vtkPlanes.h"
#include "vtkActorCollection.h"
#include "vtkPolyDataNormals.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAlgorithmOutput.h"

#include "vtkLightActor.h"
#include "vtkProcess.h"
#include "vtkTreeCompositer.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkTestErrorObserver.h"

namespace
{

// Defined in TestLightActor.cxx
// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer *r);

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();

  virtual void Execute();

  void SetArgs(int anArgc,
               char *anArgv[])
  {
      this->Argc=anArgc;
      this->Argv=anArgv;
  }

protected:
  MyProcess();

  int Argc;
  char **Argv;
};

vtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc=0;
  this->Argv=0;
}

void MyProcess::Execute()
{
  // multiprocesss logic
  int numProcs=this->Controller->GetNumberOfProcesses();
  int me=this->Controller->GetLocalProcessId();

  vtkCompositeRenderManager *prm = vtkCompositeRenderManager::New();

  vtkTreeCompositer *compositer=vtkTreeCompositer::New();
  prm->SetCompositer(compositer);
  compositer->Delete();

  vtkRenderWindowInteractor *iren=0;

  if(me==0)
  {
    iren=vtkRenderWindowInteractor::New();
  }

  vtkRenderWindow *renWin = prm->MakeRenderWindow();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);

  if(me==0)
  {
    iren->SetRenderWindow(renWin);
  }

  vtkRenderer *renderer = prm->MakeRenderer();
  renWin->AddRenderer(renderer);
  renderer->Delete();

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

  vtkNew<vtkTest::ErrorObserver> errorObserver2;
  vtkShadowMapBakerPass *shadowsBaker=vtkShadowMapBakerPass::New();
  shadowsBaker->SetOpaquePass(opaqueCameraPass);
  shadowsBaker->SetResolution(1024);
  // To cancel self-shadowing.
  shadowsBaker->SetPolygonOffsetFactor(3.1f);
  shadowsBaker->SetPolygonOffsetUnits(10.0f);
  shadowsBaker->AddObserver(
    vtkCommand::ErrorEvent, errorObserver2.GetPointer());

  vtkCompositeZPass *compositeZPass=vtkCompositeZPass::New();
  compositeZPass->SetController(this->Controller);
  shadowsBaker->SetCompositeZPass(compositeZPass);
  compositeZPass->Delete();

  vtkNew<vtkTest::ErrorObserver> errorObserver3;
  vtkShadowMapPass *shadows=vtkShadowMapPass::New();
  shadows->SetShadowMapBakerPass(shadowsBaker);
  shadows->SetOpaquePass(opaqueSequence);
  shadows->AddObserver(
    vtkCommand::ErrorEvent, errorObserver2.GetPointer());

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

  vtkOpenGLRenderer *glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
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

  vtkNew<vtkTest::ErrorObserver> errorObserver1;

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
  rectangleActor->GetProperty()->AddObserver(
    vtkCommand::ErrorEvent, errorObserver1.GetPointer());

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
  boxActor->GetProperty()->AddObserver(
    vtkCommand::ErrorEvent, errorObserver1.GetPointer());

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
  coneActor->GetProperty()->AddObserver(
    vtkCommand::ErrorEvent, errorObserver1.GetPointer());
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
  sphereActor->GetProperty()->AddObserver(
    vtkCommand::ErrorEvent, errorObserver1.GetPointer());

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
  renWin->SetSize(400,400); // 400,400
  renWin->SetPosition(0, 460*me); // translate the window
  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  // Tell the pipeline which piece we want to update.
  sphereMapper->SetNumberOfPieces( numProcs);
  sphereMapper->SetPiece(me);
  coneMapper->SetNumberOfPieces( numProcs);
  coneMapper->SetPiece(me);
  rectangleMapper->SetNumberOfPieces( numProcs);
  rectangleMapper->SetPiece(me);
  boxMapper->SetNumberOfPieces( numProcs);
  boxMapper->SetPiece(me);

  int retVal;
  const int MY_RETURN_VALUE_MESSAGE=0x518113;

  if(me>0)
  {
    // satellite nodes
    prm->StartServices(); // start listening other processes (blocking call).
    // receive return value from root process.
    this->Controller->Receive(&retVal, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }
  else
  {
    // root node
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

    if(compositeZPass->IsSupported(
             static_cast<vtkOpenGLRenderWindow *>(renWin)))
    {
      retVal=vtkTesting::Test(this->Argc, this->Argv, renWin, 10);
    }
    else
    {
      retVal=vtkTesting::PASSED; // not supported.
    }

    if(retVal==vtkRegressionTester::DO_INTERACTOR)
    {
      renWin->Render();
      iren->Start();
    }
    prm->StopServices(); // tells satellites to stop listening.

    // send the return value to the satellites
    int i=1;
    while(i<numProcs)
    {
      this->Controller->Send(&retVal, 1, i, MY_RETURN_VALUE_MESSAGE);
      ++i;
    }
    iren->Delete();
  }

  renWin->Delete();
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
  prm->Delete();
  shadowsBaker->Delete();
  this->ReturnValue=retVal;
}

// DUPLICATE for VTK/Rendering/Testing/Cxx/TestLightActor.cxx

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

}

int TestDistributedDataShadowMapPass(int argc, char *argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController *contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1; //1==failed

  vtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if(numProcs!=2)
  {
    if (me == 0)
    {
      cout << "DistributedData test requires 2 processes" << endl;
    }
    contr->Delete();
    return retVal;
  }

  if (!contr->IsA("vtkMPIController"))
  {
    if (me == 0)
    {
      cout << "DistributedData test requires MPI" << endl;
    }
    contr->Delete();
    return retVal;
  }

  MyProcess *p=MyProcess::New();
  p->SetArgs(argc,argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal=p->GetReturnValue();
  p->Delete();
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
