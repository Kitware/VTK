// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkExtractSelection.h"
#include "vtkGeometryFilter.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectionSource.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

int TestFastUnstructuredGridWithPolyDataGeometryFilter(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(0, 0, 0);
  sphere->SetRadius(0.5);
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);

  vtkNew<vtkSelectionSource> selectionSource1;
  selectionSource1->SetContentType(vtkSelectionNode::INDICES);
  selectionSource1->SetFieldType(vtkSelectionNode::CELL);
  selectionSource1->AddID(0, 0);
  selectionSource1->AddID(0, 1);
  selectionSource1->AddID(0, 2);
  selectionSource1->AddID(0, 3);
  selectionSource1->AddID(0, 4);
  selectionSource1->AddID(0, 5);
  selectionSource1->AddID(0, 6);
  selectionSource1->AddID(0, 7);
  selectionSource1->AddID(0, 8);
  selectionSource1->AddID(0, 9);
  selectionSource1->AddID(0, 10);
  selectionSource1->AddID(0, 11);
  selectionSource1->AddID(0, 12);
  selectionSource1->AddID(0, 13);
  selectionSource1->AddID(0, 14);
  selectionSource1->AddID(0, 15);
  selectionSource1->AddID(0, 32);
  selectionSource1->AddID(0, 33);
  selectionSource1->AddID(0, 58);
  selectionSource1->AddID(0, 59);
  selectionSource1->AddID(0, 84);
  selectionSource1->AddID(0, 85);
  selectionSource1->AddID(0, 110);
  selectionSource1->AddID(0, 111);
  selectionSource1->AddID(0, 136);
  selectionSource1->AddID(0, 137);
  selectionSource1->AddID(0, 162);
  selectionSource1->AddID(0, 163);
  selectionSource1->AddID(0, 188);
  selectionSource1->AddID(0, 189);
  selectionSource1->AddID(0, 214);
  selectionSource1->AddID(0, 215);
  selectionSource1->AddID(0, 240);
  selectionSource1->AddID(0, 241);
  selectionSource1->AddID(0, 266);
  selectionSource1->AddID(0, 267);
  selectionSource1->AddID(0, 292);
  selectionSource1->AddID(0, 293);
  selectionSource1->AddID(0, 318);
  selectionSource1->AddID(0, 319);
  selectionSource1->AddID(0, 344);
  selectionSource1->AddID(0, 345);
  selectionSource1->AddID(0, 370);
  selectionSource1->AddID(0, 371);
  selectionSource1->AddID(0, 396);
  selectionSource1->AddID(0, 397);
  selectionSource1->AddID(0, 422);
  selectionSource1->AddID(0, 423);

  vtkNew<vtkExtractSelection> extractSelection1;
  extractSelection1->SetInputConnection(0, sphere->GetOutputPort());
  extractSelection1->SetInputConnection(1, selectionSource1->GetOutputPort());
  extractSelection1->Update();
  auto output = vtkUnstructuredGrid::SafeDownCast(extractSelection1->GetOutput());
  if (!output->GetPointData()->HasArray("vtkOriginalPointIds"))
  {
    vtkLog(ERROR, "vtkOriginalPointIds array not found");
    return EXIT_FAILURE;
  }
  if (!output->GetCellData()->HasArray("vtkOriginalCellIds"))
  {
    vtkLog(ERROR, "vtkOriginalCellIds array not found");
    return EXIT_FAILURE;
  }

  vtkNew<vtkSelectionSource> selectionSource2;
  selectionSource2->SetContentType(vtkSelectionNode::INDICES);
  selectionSource2->SetFieldType(vtkSelectionNode::CELL);
  for (vtkIdType i = 0; i < 16; ++i)
  {
    selectionSource2->AddID(0, i);
  }

  vtkNew<vtkExtractSelection> extractSelection2;
  extractSelection2->SetInputConnection(0, extractSelection1->GetOutputPort());
  extractSelection2->SetInputConnection(1, selectionSource2->GetOutputPort());

  vtkNew<vtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(extractSelection2->GetOutputPort());
  geometryFilter->PassThroughPointIdsOn();
  geometryFilter->PassThroughCellIdsOn();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
