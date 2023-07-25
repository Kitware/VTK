// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSet.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridGeometryPassCellIds(int argc, char* argv[])
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource;
  htgSource->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource->SetCustomDim(3);
  htgSource->SetCustomFactor(3);
  htgSource->SetCustomDepth(4);
  std::vector<unsigned int> subdivs = { 3, 3, 3 };
  std::vector<double> extent = { -1, 1, -1, 1, -1, 1 };
  htgSource->SetCustomSubdivisions(subdivs.data());
  htgSource->SetCustomExtent(extent.data());

  vtkNew<vtkHyperTreeGridGeometry> geom;
  geom->SetPassThroughCellIds(true);
  geom->SetInputConnection(htgSource->GetOutputPort());
  geom->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom->GetOutputPort());

  auto ds = vtkDataSet::SafeDownCast(geom->GetOutput());

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(100);
  lut->SetTableRange(0, ds->GetNumberOfCells());

  mapper->ScalarVisibilityOn();
  mapper->SetLookupTable(lut);
  mapper->UseLookupTableScalarRangeOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->ColorByArrayComponent("vtkOriginalCellIds", 0);
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
  camera->SetPosition(-1.5, -1.5, -1.5);
  renderer->ResetCamera();

  renWin->Render();
  return !vtkRegressionTester::Test(argc, argv, renWin, 10);
}
