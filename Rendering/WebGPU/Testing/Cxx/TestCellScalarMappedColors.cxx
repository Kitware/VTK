// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkStringArray.h>

int TestCellScalarMappedColors(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  vtkNew<vtkPolyData> polydata;
  polydata->ShallowCopy(sphere->GetOutput());

  // Set up string array associated with cells
  vtkNew<vtkStringArray> sArray;
  sArray->SetName("color");
  sArray->SetNumberOfComponents(1);
  sArray->SetNumberOfTuples(polydata->GetNumberOfCells());

  vtkVariant colors[5];
  colors[0] = "red";
  colors[1] = "blue";
  colors[2] = "green";
  colors[3] = "yellow";
  colors[4] = "cyan";

  // Round-robin assignment of color strings
  for (int i = 0; i < polydata->GetNumberOfCells(); ++i)
  {
    sArray->SetValue(i, colors[i % 5].ToString());
  }

  vtkCellData* cd = polydata->GetCellData();
  cd->AddArray(sArray);

  // Set up transfer function
  vtkNew<vtkDiscretizableColorTransferFunction> tfer;
  tfer->IndexedLookupOn();
  tfer->SetNumberOfIndexedColors(5);
  tfer->SetIndexedColor(0, 1.0, 0.0, 0.0);
  tfer->SetIndexedColor(1, 0.0, 0.0, 1.0);
  tfer->SetIndexedColor(2, 0.0, 1.0, 0.0);
  tfer->SetIndexedColor(3, 1.0, 1.0, 0.0);
  tfer->SetIndexedColor(4, 0.0, 1.0, 1.0);

  std::string red("red");
  tfer->SetAnnotation(red, red);
  std::string blue("blue");
  tfer->SetAnnotation(blue, blue);
  std::string green("green");
  tfer->SetAnnotation(green, green);
  std::string yellow("yellow");
  tfer->SetAnnotation(yellow, yellow);
  std::string cyan("cyan");
  tfer->SetAnnotation(cyan, cyan);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(polydata);
  mapper->SetLookupTable(tfer);
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("color");
  mapper->DebugOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  //   if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return 0;
}
