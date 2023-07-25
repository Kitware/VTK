// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vector>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcess.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestCompositeDataProbeFilterWithHyperTreeGrid(int argc, char* argv[])
{
  vtkNew<vtkMultiBlockDataSet> sourceMBDS;
  sourceMBDS->SetNumberOfBlocks(2);

  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource0;
  htgSource0->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource0->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource0->SetCustomDim(3);
  htgSource0->SetCustomFactor(3);
  htgSource0->SetCustomDepth(5);
  std::vector<unsigned int> subdivs = { 3, 3, 3 };
  std::vector<double> extent = { -10, 0, -10, 10, -10, 10 };
  htgSource0->SetCustomSubdivisions(subdivs.data());
  htgSource0->SetCustomExtent(extent.data());

  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource1;
  htgSource1->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource1->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource1->SetCustomDim(3);
  htgSource1->SetCustomFactor(3);
  htgSource1->SetCustomDepth(6);
  subdivs[2] = 2;
  extent[0] = 0;
  extent[1] = 10;
  htgSource1->SetCustomSubdivisions(subdivs.data());
  htgSource1->SetCustomExtent(extent.data());

  htgSource0->Update();
  htgSource1->Update();

  sourceMBDS->SetBlock(0, htgSource0->GetOutput());
  sourceMBDS->SetBlock(1, htgSource1->GetOutput());

  vtkNew<vtkRTAnalyticSource> wavelet;

  vtkNew<vtkCompositeDataProbeFilter> prober;
  prober->SetInputConnection(wavelet->GetOutputPort());
  prober->SetSourceData(sourceMBDS);
  prober->SetPassPointArrays(true);
  prober->SetComputeTolerance(false);
  prober->SetTolerance(0.0);
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
