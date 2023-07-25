// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"

#include "vtkActor.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridPreConfiguredSource(int, char*[])
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> myGenerator;

  vtkNew<vtkHyperTreeGridGeometry> geom;
  geom->SetInputConnection(myGenerator->GetOutputPort());

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_2X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_3DEPTH_2BRANCH_2X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_2DEPTH_3BRANCH_3X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_4DEPTH_3BRANCH_2X2);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_3X2X3);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_2DEPTH_3BRANCH_3X3X2);

  geom->Update();

  myGenerator->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);

  geom->Update();

  myGenerator->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  myGenerator->SetCustomDim(2);
  myGenerator->SetCustomFactor(3);
  myGenerator->SetCustomDepth(4);

  geom->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom->GetOutputPort());

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfTableValues(5);
  lut->SetTableRange(0, 4);

  mapper->ScalarVisibilityOn();
  mapper->SetLookupTable(lut);
  mapper->UseLookupTableScalarRangeOn();
  mapper->SetScalarModeToUseCellFieldData();
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

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
