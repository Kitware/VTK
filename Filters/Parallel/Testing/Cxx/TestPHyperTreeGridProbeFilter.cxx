// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vector>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkLookupTable.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkPHyperTreeGridProbeFilter.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProcess.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

class MyProcess : public vtkProcess
{
public:
  static MyProcess* New();

  void Execute() override;

  void SetArgs(int anArgc, char* anArgv[]);

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

void MyProcess::SetArgs(int anArgc, char* anArgv[])
{
  this->Argc = anArgc;
  this->Argv = anArgv;
}

// The HTG should really be distributed for this to be a full
//  test of the parallel implementation
void MyProcess::Execute()
{
  this->ReturnValue = 1;

  int thisProc = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetDimensions(5, 5, 5);
  htgSource->SetOutputBounds(-10, 10, -10, 10, -10, 10);
  htgSource->SetSeed(0);
  htgSource->SetMaxDepth(4);
  htgSource->SetSplitFraction(0.4);

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkPHyperTreeGridProbeFilter> prober;
  prober->SetInputConnection(wavelet->GetOutputPort());
  prober->SetSourceConnection(htgSource->GetOutputPort());
  prober->SetPassPointArrays(true);

  prober->UpdatePiece(thisProc, numProcs, 0);
  prober->GetOutput()->GetPointData()->SetActiveScalars("Depth");

  vtkNew<vtkDataSetSurfaceFilter> geom;
  geom->SetInputConnection(prober->GetOutputPort());

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(6);
  lut->SetTableRange(0, 5);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetLookupTable(lut);
  mapper->UseLookupTableScalarRangeOn();
  mapper->SetScalarModeToUsePointData();
  mapper->ColorByArrayComponent("Depth", 0);
  mapper->InterpolateScalarsBeforeMappingOn();
  mapper->SetNumberOfPieces(numProcs);
  mapper->SetPiece(thisProc);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToSurface();
  actor->GetProperty()->EdgeVisibilityOn();

  // For distributed rendering
  vtkNew<vtkCompositeRenderManager> crm;

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::Take(crm->MakeRenderer());
  renderer->AddActor(actor);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::Take(crm->MakeRenderWindow());
  renWin->AddRenderer(renderer.Get());
  // Antialiasing prevent distributed pipeline
  renWin->SetMultiSamples(0);

  crm->SetRenderWindow(renWin);
  crm->SetController(this->Controller);

  const int MY_RETURN_VALUE_MESSAGE = 0x42;

  if (thisProc == 0)
  {
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetPosition(-15, -15, -15);
    // All camera should be reset. If not, only root node
    // will update the pipeline, and reducing operation in
    // vtkPHyperTreeGridProbeFilter will be blocked.
    crm->ResetAllCameras();

    renWin->Render();
    this->ReturnValue = vtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);
    crm->StopServices();

    for (int i = 1; i < numProcs; i++)
    {
      this->Controller->Send(&this->ReturnValue, 1, i, MY_RETURN_VALUE_MESSAGE);
    }
  }
  else
  {
    crm->StartServices();
    this->Controller->Receive(&this->ReturnValue, 1, 0, MY_RETURN_VALUE_MESSAGE);
  }
}

int TestPHyperTreeGridProbeFilter(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  vtkNew<MyProcess> p;
  p->SetArgs(argc, argv);
  controller->SetSingleProcessObject(p);
  controller->SingleMethodExecute();

  int retVal = p->GetReturnValue();

  controller->Finalize();
  return !retVal;
}
