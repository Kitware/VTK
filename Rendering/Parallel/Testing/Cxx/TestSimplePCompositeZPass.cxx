// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCompositeRenderManager.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include <vtk_mpi.h>

#include "vtkRegressionTestImage.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkImageSinusoidSource.h"
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper.h"

#include "vtkActorCollection.h"
#include "vtkCameraPass.h"
#include "vtkCompositeZPass.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkDepthPeelingPass.h"
#include "vtkFrustumSource.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLightsPass.h"
#include "vtkMath.h"
#include "vtkOpaquePass.h"
#include "vtkOverlayPass.h"
#include "vtkPlaneSource.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkSequencePass.h"
#include "vtkSphereSource.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"
#include <cassert>

#include "vtkLightActor.h"
#include "vtkProcess.h"

#include "vtkImageAppendComponents.h"
#include "vtkImageImport.h"
#include "vtkImageShiftScale.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

namespace
{

// Defined in TestLightActor.cxx
// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer* r);

class MyProcess : public vtkProcess
{
public:
  static MyProcess* New();

  void Execute() override;

  void SetArgs(int anArgc, char* anArgv[])
  {
    this->Argc = anArgc;
    this->Argv = anArgv;
  }

protected:
  MyProcess();

  int Argc;
  char** Argv;
};

vtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc = 0;
  this->Argv = nullptr;
}

void MyProcess::Execute()
{
  // multiprocesss logic
  int numProcs = this->Controller->GetNumberOfProcesses();
  int me = this->Controller->GetLocalProcessId();

  vtkCompositeRenderManager* prm = vtkCompositeRenderManager::New();

  vtkRenderWindowInteractor* iren = nullptr;

  if (me == 0)
  {
    iren = vtkRenderWindowInteractor::New();
  }

  vtkRenderWindow* renWin = prm->MakeRenderWindow();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);

  if (me == 0)
  {
    iren->SetRenderWindow(renWin);
  }

  vtkRenderer* renderer = prm->MakeRenderer();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  vtkCameraPass* cameraP = vtkCameraPass::New();

  vtkOpaquePass* opaque = vtkOpaquePass::New();

  vtkLightsPass* lights = vtkLightsPass::New();

  VTK_CREATE(vtkTest::ErrorObserver, errorObserver);
  vtkCompositeZPass* compositeZPass = vtkCompositeZPass::New();
  compositeZPass->SetController(this->Controller);
  compositeZPass->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkSequencePass* seq = vtkSequencePass::New();
  vtkRenderPassCollection* passes = vtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  passes->AddItem(compositeZPass);
  compositeZPass->Delete();

  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);

  vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
  glrenderer->SetPass(cameraP);

  vtkPlaneSource* rectangleSource = vtkPlaneSource::New();
  rectangleSource->SetOrigin(-5.0, 0.0, 5.0);
  rectangleSource->SetPoint1(5.0, 0.0, 5.0);
  rectangleSource->SetPoint2(-5.0, 0.0, -5.0);
  rectangleSource->SetResolution(100, 100);

  vtkPolyDataMapper* rectangleMapper = vtkPolyDataMapper::New();
  rectangleMapper->SetInputConnection(rectangleSource->GetOutputPort());
  rectangleSource->Delete();
  rectangleMapper->SetScalarVisibility(0);

  vtkActor* rectangleActor = vtkActor::New();
  rectangleActor->SetMapper(rectangleMapper);
  rectangleMapper->Delete();
  rectangleActor->SetVisibility(1);
  rectangleActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  vtkCubeSource* boxSource = vtkCubeSource::New();
  boxSource->SetXLength(2.0);
  vtkPolyDataNormals* boxNormals = vtkPolyDataNormals::New();
  boxNormals->SetInputConnection(boxSource->GetOutputPort());
  boxNormals->SetComputePointNormals(0);
  boxNormals->SetComputeCellNormals(1);
  boxNormals->Update();
  boxNormals->GetOutput()->GetPointData()->SetNormals(nullptr);

  vtkPolyDataMapper* boxMapper = vtkPolyDataMapper::New();
  boxMapper->SetInputConnection(boxNormals->GetOutputPort());
  boxNormals->Delete();
  boxSource->Delete();
  boxMapper->SetScalarVisibility(0);

  vtkActor* boxActor = vtkActor::New();

  boxActor->SetMapper(boxMapper);
  boxMapper->Delete();
  boxActor->SetVisibility(1);
  boxActor->SetPosition(-2.0, 2.0, 0.0);
  boxActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  vtkConeSource* coneSource = vtkConeSource::New();
  coneSource->SetResolution(24);
  coneSource->SetDirection(1.0, 1.0, 1.0);
  vtkPolyDataMapper* coneMapper = vtkPolyDataMapper::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  coneSource->Delete();
  coneMapper->SetScalarVisibility(0);

  vtkActor* coneActor = vtkActor::New();
  coneActor->SetMapper(coneMapper);
  coneMapper->Delete();
  coneActor->SetVisibility(1);
  coneActor->SetPosition(0.0, 1.0, 1.0);
  coneActor->GetProperty()->SetColor(0.0, 0.0, 1.0);
  //  coneActor->GetProperty()->SetLighting(false);

  vtkSphereSource* sphereSource = vtkSphereSource::New();
  sphereSource->SetThetaResolution(32);
  sphereSource->SetPhiResolution(32);
  vtkPolyDataMapper* sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereSource->Delete();
  sphereMapper->SetScalarVisibility(0);

  vtkActor* sphereActor = vtkActor::New();
  sphereActor->SetMapper(sphereMapper);
  sphereMapper->Delete();
  sphereActor->SetVisibility(1);
  sphereActor->SetPosition(2.0, 2.0, -1.0);
  sphereActor->GetProperty()->SetColor(1.0, 1.0, 0.0);

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
  vtkLight* l1 = vtkLight::New();
  l1->SetPosition(-4.0, 4.0, -1.0);
  l1->SetFocalPoint(boxActor->GetPosition());
  l1->SetColor(1.0, 1.0, 1.0);
  l1->SetPositional(1);
  renderer->AddLight(l1);
  l1->SetSwitch(1);
  l1->Delete();

  // lighting the sphere
  vtkLight* l2 = vtkLight::New();
  l2->SetPosition(4.0, 5.0, 1.0);
  l2->SetFocalPoint(sphereActor->GetPosition());
  l2->SetColor(1.0, 0.0, 1.0);
  //  l2->SetColor(1.0,1.0,1.0);
  l2->SetPositional(1);
  renderer->AddLight(l2);
  l2->SetSwitch(1);
  l2->Delete();

  AddLightActors(renderer);

  renderer->SetBackground(0.66, 0.66, 0.66);
  renderer->SetBackground2(157.0 / 255.0 * 0.66, 186 / 255.0 * 0.66, 192.0 / 255.0 * 0.66);
  renderer->SetGradientBackground(true);
  renWin->SetSize(400, 400);        // 400,400
  renWin->SetPosition(0, 460 * me); // translate the window
  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  if (me == 0)
  {
    rectangleActor->SetVisibility(false);
    boxActor->SetVisibility(false);
  }
  else
  {
    coneActor->SetVisibility(false);
    sphereActor->SetVisibility(false);
  }

  int retVal;
  const int MY_RETURN_VALUE_MESSAGE = 0xcafe;

  if (me > 0)
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
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->Azimuth(40.0);
    camera->Elevation(10.0);
    renderer->ResetCamera();
    // testing code
    double thresh = 0.05;
    int i;
    VTK_CREATE(vtkTesting, testing);
    for (i = 0; i < this->Argc; ++i)
    {
      testing->AddArgument(this->Argv[i]);
    }

    if (testing->IsInteractiveModeSpecified())
    {
      retVal = vtkTesting::DO_INTERACTOR;
    }
    else
    {
      if (testing->IsValidImageSpecified())
      {
        renWin->Render();
        if (compositeZPass->IsSupported(static_cast<vtkOpenGLRenderWindow*>(renWin)))
        {
          int* dims;
          dims = renWin->GetSize();
          float* zBuffer = new float[dims[0] * dims[1]];
          renWin->GetZbufferData(0, 0, dims[0] - 1, dims[1] - 1, zBuffer);

          vtkImageImport* importer = vtkImageImport::New();
          size_t byteSize = static_cast<size_t>(dims[0] * dims[1]);
          byteSize = byteSize * sizeof(float);
          importer->CopyImportVoidPointer(zBuffer, static_cast<int>(byteSize));
          importer->SetDataScalarTypeToFloat();
          importer->SetNumberOfScalarComponents(1);
          importer->SetWholeExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, 0);
          importer->SetDataExtentToWholeExtent();

          vtkImageShiftScale* converter = vtkImageShiftScale::New();
          converter->SetInputConnection(importer->GetOutputPort());
          converter->SetOutputScalarTypeToUnsignedChar();
          converter->SetShift(0.0);
          converter->SetScale(255.0);

          // vtkImageDifference requires 3 components.
          vtkImageAppendComponents* luminanceToRGB = vtkImageAppendComponents::New();
          luminanceToRGB->SetInputConnection(0, converter->GetOutputPort());
          luminanceToRGB->AddInputConnection(0, converter->GetOutputPort());
          luminanceToRGB->AddInputConnection(0, converter->GetOutputPort());
          luminanceToRGB->Update();

          retVal = testing->RegressionTest(luminanceToRGB, thresh);

          luminanceToRGB->Delete();
          converter->Delete();
          importer->Delete();
          delete[] zBuffer;
        }
        else
        {
          retVal = vtkTesting::PASSED; // not supported.
        }
      }
      else
      {
        retVal = vtkTesting::NOT_RUN;
      }
    }

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    prm->StopServices(); // tells satellites to stop listening.

    // send the return value to the satellites
    i = 1;
    while (i < numProcs)
    {
      this->Controller->Send(&retVal, 1, i, MY_RETURN_VALUE_MESSAGE);
      ++i;
    }
    iren->Delete();
  }

  renWin->Delete();
  opaque->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  prm->Delete();
  this->ReturnValue = retVal;
}

// DUPLICATE for VTK/Rendering/Testing/Cxx/TestLightActor.cxx

// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer* r)
{
  assert("pre: r_exists" && r != nullptr);

  vtkLightCollection* lights = r->GetLights();

  lights->InitTraversal();
  vtkLight* l = lights->GetNextItem();
  while (l != nullptr)
  {
    double angle = l->GetConeAngle();
    if (l->LightTypeIsSceneLight() && l->GetPositional() && angle < 90.0) // spotlight
    {
      vtkLightActor* la = vtkLightActor::New();
      la->SetLight(l);
      r->AddViewProp(la);
      la->Delete();
    }
    l = lights->GetNextItem();
  }
}

}

int TestSimplePCompositeZPass(int argc, char* argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = 1; // 1==failed

  vtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if (numProcs != 2)
  {
    if (me == 0)
    {
      cout << "TestSimplePCompositeZPass test requires 2 processes" << endl;
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

  MyProcess* p = MyProcess::New();
  p->SetArgs(argc, argv);

  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal = p->GetReturnValue();
  p->Delete();
  contr->Finalize();
  contr->Delete();

  return !retVal;
}
