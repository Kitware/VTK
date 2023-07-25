// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vector>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkLookupTable.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkPHyperTreeGridProbeFilter.h"
#include "vtkPointData.h"
#include "vtkProcess.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
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

  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource;
  htgSource->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource->SetCustomDim(3);
  htgSource->SetCustomFactor(3);
  htgSource->SetCustomDepth(5);
  std::vector<unsigned int> subdivs = { 3, 3, 3 };
  std::vector<double> extent = { -10, 10, -10, 10, -10, 10 };
  htgSource->SetCustomSubdivisions(subdivs.data());
  htgSource->SetCustomExtent(extent.data());

  vtkNew<vtkRTAnalyticSource> wavelet;

  vtkNew<vtkPHyperTreeGridProbeFilter> prober;
  prober->SetInputConnection(wavelet->GetOutputPort());
  prober->SetSourceConnection(htgSource->GetOutputPort());
  prober->SetPassPointArrays(true);

  prober->Update();
  prober->GetOutput()->GetPointData()->SetActiveScalars("Depth");

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(prober->GetOutputPort());

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(6);
  lut->SetTableRange(0, 5);

  mapper->ScalarVisibilityOn();
  mapper->SetLookupTable(lut);
  mapper->UseLookupTableScalarRangeOn();
  mapper->SetScalarModeToUsePointData();
  mapper->ColorByArrayComponent("Depth", 0);
  mapper->InterpolateScalarsBeforeMappingOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToSurface();
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer.Get());

  const int MY_RETURN_VALUE_MESSAGE = 0x42;

  if (thisProc == 0)
  {
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->SetPosition(-15, -15, -15);
    renderer->ResetCamera();

    renWin->Render();
    this->ReturnValue = vtkRegressionTester::Test(this->Argc, this->Argv, renWin, 10);

    for (int i = 1; i < numProcs; i++)
    {
      this->Controller->Send(&this->ReturnValue, 1, i, MY_RETURN_VALUE_MESSAGE);
    }
  }
  else
  {
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
