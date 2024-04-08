// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtk3DLinearGridPlaneCutter.h>
#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkGenerateIds.h>
#include <vtkNew.h>
#include <vtkPassArrays.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkXMLUnstructuredGridReader.h>

int Test3DLinearGridPlaneCutterCellData(int argc, char* argv[])
{
  const auto cellArrayName = "CellIds";

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/slightlyRotated.vtu");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0, 1.0, 0.5);

  // add simple cell data
  vtkNew<vtkGenerateIds> computeIds;
  computeIds->SetInputConnection(reader->GetOutputPort());
  computeIds->SetPointIds(false);
  computeIds->SetCellIds(true);
  computeIds->SetCellIdsArrayName(cellArrayName);
  computeIds->Update();

  vtkNew<vtkPassArrays> removeArrays;
  removeArrays->SetInputConnection(computeIds->GetOutputPort());

  // create plane cutter
  vtkNew<vtk3DLinearGridPlaneCutter> slicer;
  slicer->SetInputConnection(removeArrays->GetOutputPort());
  slicer->SetPlane(plane);
  slicer->SetInterpolateAttributes(true);
  slicer->SetMergePoints(false);
  slicer->Update();

  // smoke test with no point data
  removeArrays->ClearPointDataArrays();
  slicer->Update();

  // smoke test with no point and no cell data
  removeArrays->ClearCellDataArrays();
  slicer->Update();

  // smoke test with cell data only
  removeArrays->AddCellDataArray(cellArrayName);
  slicer->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(slicer->GetOutputPort());
  mapper->SetScalarModeToUseCellData();
  mapper->SetColorModeToMapScalars();
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray(cellArrayName);

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
