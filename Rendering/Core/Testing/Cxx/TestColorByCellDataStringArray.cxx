/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorByCellDataStringArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkStdString.h>
#include <vtkStringArray.h>


int TestColorByCellDataStringArray(int argc, char* argv[])
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
  cd->AddArray(sArray.Get());

  // Set up transfer function
  vtkNew<vtkDiscretizableColorTransferFunction> tfer;
  tfer->IndexedLookupOn();
  tfer->SetNumberOfIndexedColors(5);
  tfer->SetIndexedColor(0, 1.0, 0.0, 0.0);
  tfer->SetIndexedColor(1, 0.0, 0.0, 1.0);
  tfer->SetIndexedColor(2, 0.0, 1.0, 0.0);
  tfer->SetIndexedColor(3, 1.0, 1.0, 0.0);
  tfer->SetIndexedColor(4, 0.0, 1.0, 1.0);

  vtkStdString red("red");
  tfer->SetAnnotation(red, red);
  vtkStdString blue("blue");
  tfer->SetAnnotation(blue, blue);
  vtkStdString green("green");
  tfer->SetAnnotation(green, green);
  vtkStdString yellow("yellow");
  tfer->SetAnnotation(yellow, yellow);
  vtkStdString cyan("cyan");
  tfer->SetAnnotation(cyan, cyan);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(polydata.Get());
  mapper->SetLookupTable(tfer.Get());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("color");

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.Get());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.Get());

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow.Get());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
