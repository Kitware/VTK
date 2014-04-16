/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests client-server rendering using the vtkClientServerCompositePass.

#include "vtkObjectFactory.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkClearZPass.h"
#include "vtkClientServerCompositePass.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDepthPeelingPass.h"
#include "vtkDistributedDataFilter.h"
#include "vtkImageRenderManager.h"
#include "vtkLightsPass.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOverlayPass.h"
#include "vtkPieceScalars.h"
#include "vtkPKdTree.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkSphereSource.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkTestUtilities.h"
#include "vtkTranslucentPass.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumetricPass.h"
#include "vtkNew.h"
#include "vtkTesting.h"

#include <vtksys/CommandLineArguments.hxx>

namespace
{

class MyProcess : public vtkObject
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkObject);
  vtkSetMacro(ImageReductionFactor, int);
  // Returns true on success.
  bool Execute(int argc, char** argv);

  // Get/Set the controller.
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  bool IsServer;
private:
  // Creates the visualization pipeline and adds it to the renderer.
  void CreatePipeline(vtkRenderer* renderer);

  // Setups render passes.
  void SetupRenderPasses(vtkRenderer* renderer);

protected:
  MyProcess();
  ~MyProcess();
  int ImageReductionFactor;
  vtkMultiProcessController* Controller;
};

vtkStandardNewMacro(MyProcess);

//-----------------------------------------------------------------------------
MyProcess::MyProcess()
{
  this->ImageReductionFactor = 1;
  this->Controller = NULL;
}

//-----------------------------------------------------------------------------
MyProcess::~MyProcess()
{
  this->SetController(NULL);
}

//-----------------------------------------------------------------------------
void MyProcess::CreatePipeline(vtkRenderer* renderer)
{
  double bounds[] = {-0.5, .5, -0.5, .5, -0.5, 0.5};
  renderer->ResetCamera(bounds);
  if (!this->IsServer)
    {
    return;
    }

  vtkSphereSource* sphere = vtkSphereSource::New();
  //sphere->SetPhiResolution(100);
  //sphere->SetThetaResolution(100);

  vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
  surface->SetInputConnection(sphere->GetOutputPort());

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(surface->GetOutputPort());

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  actor->Delete();
  mapper->Delete();
  surface->Delete();
  sphere->Delete();
}

//-----------------------------------------------------------------------------
void MyProcess::SetupRenderPasses(vtkRenderer* renderer)
{
  // the rendering passes
  vtkCameraPass *cameraP=vtkCameraPass::New();
  vtkSequencePass *seq=vtkSequencePass::New();
  vtkOpaquePass *opaque=vtkOpaquePass::New();

  vtkTranslucentPass *translucent=vtkTranslucentPass::New();

  vtkVolumetricPass *volume=vtkVolumetricPass::New();
  vtkOverlayPass *overlay=vtkOverlayPass::New();
  vtkLightsPass *lights=vtkLightsPass::New();

  vtkClearZPass *clearZ=vtkClearZPass::New();
  clearZ->SetDepth(0.9);

  vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  //  passes->AddItem(clearZ);
  passes->AddItem(translucent);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);


  vtkClientServerCompositePass* csPass = vtkClientServerCompositePass::New();
  csPass->SetRenderPass(seq);
  csPass->SetProcessIsServer(this->IsServer);
  csPass->ServerSideRenderingOn();
  csPass->SetController(this->Controller);

  vtkOpenGLRenderer *glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  cameraP->SetDelegatePass(csPass);
  glrenderer->SetPass(cameraP);

  // setting viewport doesn't work in tile-display mode correctly yet.
  //renderer->SetViewport(0, 0, 0.75, 1);

  opaque->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  clearZ->Delete();
  csPass->Delete();
}

//-----------------------------------------------------------------------------
bool MyProcess::Execute(int argc, char** argv)
{
  vtkRenderWindow* renWin = vtkRenderWindow::New();

  renWin->SetWindowName(this->IsServer?
    "Server Window" : "Client Window");

  // enable alpha bit-planes.
  renWin->AlphaBitPlanesOn();

  // use double bufferring.
  renWin->DoubleBufferOn();

  // don't waste time swapping buffers unless needed.
  renWin->SwapBuffersOff();


  vtkRenderer* renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);

  vtkSynchronizedRenderWindows* syncWindows =
    vtkSynchronizedRenderWindows::New();
  syncWindows->SetRenderWindow(renWin);
  syncWindows->SetParallelController(this->Controller);
  syncWindows->SetIdentifier(2);
  syncWindows->SetRootProcessId(this->IsServer? 1 : 0);

  vtkSynchronizedRenderers* syncRenderers = vtkSynchronizedRenderers::New();
  syncRenderers->SetRenderer(renderer);
  syncRenderers->SetParallelController(this->Controller);
  syncRenderers->SetRootProcessId(this->IsServer? 1 : 0);
  syncRenderers->SetImageReductionFactor(this->ImageReductionFactor);

  this->CreatePipeline(renderer);
  this->SetupRenderPasses(renderer);

  bool success = true;
  if (!this->IsServer)
    {
    // CLIENT
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    renWin->SwapBuffersOn();
    renWin->Render();

    // regression test is done on the client since the data is on the server.
    int result = vtkTesting::Test(argc, argv, renWin, 15);
    success = (result == vtkTesting::PASSED);
    if (result == vtkTesting::DO_INTERACTOR)
      {
      iren->Start();
      }
    iren->Delete();
    this->Controller->TriggerBreakRMIs();
    }
  else
    {
    // SERVER
    this->Controller->ProcessRMIs();
    }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
  return success;
}

}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  int image_reduction_factor = 1;
  int is_server = 0;
  int port = 11111;

  vtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.StoreUnusedArguments(true);
  args.AddArgument("--image-reduction-factor",
    vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &image_reduction_factor, "Image reduction factor");
  args.AddArgument("-irf",
    vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &image_reduction_factor, "Image reduction factor (shorthand)");
  args.AddArgument("--server",
    vtksys::CommandLineArguments::NO_ARGUMENT,
    &is_server, "process is a server");
  args.AddArgument("--port",
    vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &port, "Port number (default is 11111)");
  if (!args.Parse())
    {
    return 1;
    }

  vtkSmartPointer<vtkSocketController> contr =
    vtkSmartPointer<vtkSocketController>::New();
  contr->Initialize(&argc, &argv);
  if (is_server)
    {
    cout << "Waiting for client on " << port << endl;
    contr->WaitForConnection(port);
    }
  else
    {
    if (!contr->ConnectTo(const_cast<char*>("localhost"), port))
      {
      return 1;
      }
    }

  MyProcess *p=MyProcess::New();
  p->IsServer = is_server != 0;
  p->SetImageReductionFactor(image_reduction_factor);
  p->SetController(contr);
  bool success = p->Execute(argc, argv);
  p->Delete();
  contr->Finalize();
  contr = 0;
  return success? EXIT_SUCCESS :EXIT_FAILURE;
}

