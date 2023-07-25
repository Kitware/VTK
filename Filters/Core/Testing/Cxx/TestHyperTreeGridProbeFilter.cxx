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
#include "vtkHyperTreeGridProbeFilter.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcess.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridProbeFilter(int argc, char* argv[])
{
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

  vtkNew<vtkHyperTreeGridProbeFilter> prober;
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

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-15, -15, -15);
  renderer->ResetCamera();

  renWin->Render();
  return !vtkRegressionTester::Test(argc, argv, renWin, 10);
}
