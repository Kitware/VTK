/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDebugLeaks.h"
#include "vtkLineSource.h"
#include "vtkLookupTable.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkPStreamTracer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkGeometryFilter.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkTrivialProducer.h"

struct PStreamArgs_tmp
{
  int* retVal;
  int argc;
  char** argv;
};

// This will be called by all processes
void MyMain( vtkMultiProcessController *controller, void *arg )
{

  PStreamArgs_tmp* args =
    reinterpret_cast<PStreamArgs_tmp*>(arg);

  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  vtkRenderer* ren = vtkRenderer::New();
  ren->SetBackground(0.33, 0.35, 0.43);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 300);
  renWin->SetPosition(0, 350 * myId);

  //camera parameters
  vtkCamera* camera = ren->GetActiveCamera();
  camera->SetPosition(-5.86786, 49.2857, 51.597);
  camera->SetFocalPoint(8.255, -3.17482e-16, 29.7631);
  camera->SetViewUp(-0.112182, -0.42918, 0.896225);
  camera->SetViewAngle(30);
  camera->SetClippingRange(10.0, 80.6592);
  camera->Dolly(1.5);

  // Create the reader, the data file name might have
  // to be changed depending on where the data files are.
  char* fname1 = vtkTestUtilities::ExpandDataFileName(args->argc, args->argv,
                                                     "Data/combxyz.bin");
  char* fname2 = vtkTestUtilities::ExpandDataFileName(args->argc, args->argv,
                                                     "Data/combq.bin");
  vtkMultiBlockPLOT3DReader* Plot3D0 = vtkMultiBlockPLOT3DReader::New();
  Plot3D0->SetFileName(fname1);
  Plot3D0->SetQFileName (fname2);
  Plot3D0->SetBinaryFile(1);
  Plot3D0->SetMultiGrid(0);
  Plot3D0->SetHasByteCount(0);
  Plot3D0->SetIBlanking(0);
  Plot3D0->SetTwoDimensionalGeometry(0);
  Plot3D0->SetForceRead(0);
  Plot3D0->SetByteOrder(0);
  delete[] fname1;
  delete[] fname2;
  Plot3D0->Update();

  vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(
    Plot3D0->GetOutput()->GetBlock(0));

  vtkTrivialProducer* tv = vtkTrivialProducer::New();
  tv->SetOutput(sg);

  vtkStructuredGridOutlineFilter* Geometry5 =
    vtkStructuredGridOutlineFilter::New();
  Geometry5->SetInputConnection(tv->GetOutputPort());

  vtkPolyDataMapper* Mapper5 = vtkPolyDataMapper::New();
  Mapper5->SetInputConnection(Geometry5->GetOutputPort());
  Mapper5->SetImmediateModeRendering(1);
  Mapper5->UseLookupTableScalarRangeOn();
  Mapper5->SetScalarVisibility(0);
  Mapper5->SetScalarModeToDefault();

  vtkActor* Actor5 = vtkActor::New();
  Actor5->SetMapper(Mapper5);
  vtkProperty* prop = Actor5->GetProperty();
  prop->SetRepresentationToSurface();
  prop->SetInterpolationToGouraud();
  prop->SetAmbient(0.15);
  prop->SetDiffuse(0.85);
  prop->SetSpecular(0.1);
  prop->SetSpecularPower(100);
  prop->SetSpecularColor(1, 1, 1);
  prop->SetColor(1, 1, 1);

  ren->AddActor(Actor5);

  vtkLineSource* LineSourceWidget0 = vtkLineSource::New();
  LineSourceWidget0->SetPoint1(13.9548, -0.47371, 31.7642);
  LineSourceWidget0->SetPoint2(6.3766, -0.5886, 26.6274);
  LineSourceWidget0->SetResolution(20);

  vtkPStreamTracer* Stream0 = vtkPStreamTracer::New();;
  Stream0->SetInputConnection(tv->GetOutputPort());
  Stream0->SetSourceConnection(LineSourceWidget0->GetOutputPort());
  Stream0->SetIntegrationStepUnit(2);
  Stream0->SetMaximumPropagation(5);
  Stream0->SetInitialIntegrationStep(0.5);
  Stream0->SetIntegrationDirection(2);
  Stream0->SetIntegratorType(0);
  Stream0->SetMaximumNumberOfSteps(2000);
  Stream0->SetTerminalSpeed(1e-12);

  vtkGeometryFilter* Geometry6 = vtkGeometryFilter::New();;
  Geometry6->SetInputConnection(Stream0->GetOutputPort());

  vtkLookupTable* LookupTable1 = vtkLookupTable::New();
  LookupTable1->SetNumberOfTableValues(256);
  LookupTable1->SetHueRange(0, 0.66667);
  LookupTable1->SetSaturationRange(1, 1);
  LookupTable1->SetValueRange(1, 1);
  LookupTable1->SetTableRange(0.197813, 0.710419);
  LookupTable1->SetVectorComponent(0);
  LookupTable1->Build();

  vtkPolyDataMapper* Mapper6 = vtkPolyDataMapper::New();
  Mapper6->SetInputConnection(Geometry6->GetOutputPort());
  Mapper6->SetImmediateModeRendering(1);
  Mapper6->UseLookupTableScalarRangeOn();
  Mapper6->SetScalarVisibility(1);
  Mapper6->SetScalarModeToUsePointFieldData();
  Mapper6->SelectColorArray("Density");
  Mapper6->SetLookupTable(LookupTable1);

  vtkActor* Actor6 = vtkActor::New();
  Actor6->SetMapper(Mapper6);
  prop = Actor6->GetProperty();
  prop->SetRepresentationToSurface();
  prop->SetInterpolationToGouraud();
  prop->SetAmbient(0.15);
  prop->SetDiffuse(0.85);
  prop->SetSpecular(0);
  prop->SetSpecularPower(1);
  prop->SetSpecularColor(1, 1, 1);

  ren->AddActor(Actor6);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkCompositeRenderManager* compManager = vtkCompositeRenderManager::New();
  compManager->SetRenderWindow(renWin);
  compManager->SetController(controller);
  compManager->InitializePieces();

  if (myId)
    {
    compManager->InitializeRMIs();
    controller->ProcessRMIs();
    controller->Receive(args->retVal, 1, 0, 33);
    }
  else
    {
    renWin->Render();
    *(args->retVal) =
      vtkRegressionTester::Test(args->argc, args->argv, renWin, 10);
    for (int i = 1; i < numProcs; i++)
      {
      controller->TriggerRMI(i, vtkMultiProcessController::BREAK_RMI_TAG);
      controller->Send(args->retVal, 1, i, 33);
      }
    }

  if ( *(args->retVal) == vtkRegressionTester::DO_INTERACTOR)
    {
    compManager->StartInteractor();
    }
  renWin->Delete();
  ren->Delete();
  iren->Delete();
  compManager->Delete();
  Plot3D0->Delete();
  tv->Delete();
  Stream0->Delete();
  LookupTable1->Delete();
  LineSourceWidget0->Delete();
  Geometry5->Delete();
  Geometry6->Delete();
  Actor5->Delete();
  Actor6->Delete();
  Mapper5->Delete();
  Mapper6->Delete();
}

int main( int argc, char* argv[] )
{
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);
  contr->CreateOutputWindow();

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (contr->IsA("vtkThreadedController"))
    {
    // Set the number of processes to 2 for this example.
    contr->SetNumberOfProcesses(2);
    }

  // Added for regression test.
  // ----------------------------------------------
  int retVal;
  PStreamArgs_tmp args;
  args.retVal = &retVal;
  args.argc = argc;
  args.argv = argv;
  // ----------------------------------------------

  contr->SetSingleMethod(MyMain, &args);
  contr->SingleMethodExecute();

  contr->Finalize();
  contr->Delete();

  return !retVal;
}
