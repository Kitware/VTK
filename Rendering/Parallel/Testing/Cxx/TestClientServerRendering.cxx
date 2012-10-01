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
#include "vtkProcess.h"
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

#include <vtksys/CommandLineArguments.hxx>

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkProcess);

  bool IsServer;
  vtkSetMacro(ImageReductionFactor, int);

  virtual void Execute();
private:
  // Creates the visualization pipeline and adds it to the renderer.
  void CreatePipeline(vtkRenderer* renderer);

  // Setups render passes.
  void SetupRenderPasses(vtkRenderer* renderer);

protected:
  MyProcess();
  int ImageReductionFactor;
};

vtkStandardNewMacro(MyProcess);

//-----------------------------------------------------------------------------
MyProcess::MyProcess()
{
  this->ImageReductionFactor = 1;
}

//-----------------------------------------------------------------------------
void MyProcess::CreatePipeline(vtkRenderer* renderer)
{
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
void MyProcess::Execute()
{
  vtkRenderWindow* renWin = vtkRenderWindow::New();
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

  if (!this->IsServer)
    {
    vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);
    renWin->SwapBuffersOn();
    renWin->Render();

    iren->Start();
    iren->Delete();

    this->Controller->TriggerBreakRMIs();
    }
  else
    {
    this->Controller->ProcessRMIs();
    }

  renderer->Delete();
  renWin->Delete();
  syncWindows->Delete();
  syncRenderers->Delete();
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  int retVal = 1;

  int image_reduction_factor = 1;
  int is_server = 0;
  int port = 11111;

  vtksys::CommandLineArguments args;
  args.Initialize(argc, argv);
  args.StoreUnusedArguments(false);
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
    cout << "Waiting for client on 11111" << endl;
    contr->WaitForConnection(11111);
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
  p->Execute();

  retVal=p->GetReturnValue();
  p->Delete();
  contr->Finalize();
  contr = 0;
  return !retVal;
}

