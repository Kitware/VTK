// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"

int TestScalarModeToggle(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1.0);
  sphere->SetCenter(0, 0, 0);

  // generate elevation data.
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());

  elev->Update();
  vtkPolyData* polydata = elev->GetPolyDataOutput();
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
  sArray->SetName("colors");
  cd->AddArray(sArray);
  cd->SetActiveScalars("colors");

  // map elevation output to graphics primitives.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  mapper->SetScalarModeToUsePointData();
  mapper->Update();
  mapper->SetStatic(1);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

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

  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetLookupTable(tfer);
  mapper->SelectColorArray("colors");
  renWin->Render();

  iren->Start();
  return 0;
}
