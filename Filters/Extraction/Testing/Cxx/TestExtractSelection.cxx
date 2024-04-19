// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectionSource.h"
#include "vtkSphereSource.h"

#include "vtkRegressionTestImage.h"

int TestExtractSelection(int argc, char* argv[])
{
  vtkNew<vtkSelectionSource> selection;
  selection->SetContentType(vtkSelectionNode::INDICES);
  selection->SetFieldType(vtkSelectionNode::CELL);
  selection->AddID(-1, 2);
  selection->AddID(-1, 4);
  selection->AddID(-1, 5);
  selection->AddID(-1, 8);

  vtkNew<vtkSphereSource> sphere;

  vtkNew<vtkExtractSelection> selFilter;
  selFilter->SetInputConnection(0, sphere->GetOutputPort());
  selFilter->SetInputConnection(1, selection->GetOutputPort());

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(selFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
