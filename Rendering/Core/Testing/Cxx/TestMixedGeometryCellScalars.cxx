// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkArrayCalculator.h"
#include "vtkColorTransferFunction.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRegularPolygonSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include <initializer_list>
#include <string>

namespace
{
vtkSmartPointer<vtkAlgorithm> AddCellScalarArray(
  const std::string expression, vtkSmartPointer<vtkAlgorithm> input)
{
  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetAttributeTypeToCellData();
  calculator->SetFunction(expression.c_str());
  calculator->SetInputConnection(input->GetOutputPort());
  return calculator;
}

vtkSmartPointer<vtkAlgorithm> Append(
  std::initializer_list<vtkSmartPointer<vtkAlgorithm>> inputAlgorithms)
{
  vtkNew<vtkAppendPolyData> append;
  for (const auto& inputAlgorithm : inputAlgorithms)
  {
    append->AddInputConnection(inputAlgorithm->GetOutputPort());
  }
  return append;
}
}

int TestMixedGeometryCellScalars(int argc, char* argv[])
{
  vtkNew<vtkPolyPointSource> points;
  points->SetNumberOfPoints(5);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 0, 0);
  points->SetPoint(2, 2, 1, 0);
  points->SetPoint(3, 2, 2, 0);
  points->SetPoint(4, 1, 3, 0);

  vtkNew<vtkPolyLineSource> polyline;
  polyline->SetClosed(false);
  polyline->SetNumberOfPoints(5);
  polyline->SetPoint(0, 0, 0, 0);
  polyline->SetPoint(1, 1, 0, 0);
  polyline->SetPoint(2, 2, 1, 0);
  polyline->SetPoint(3, 2, 2, 0);
  polyline->SetPoint(4, 1, 3, 0);

  vtkNew<vtkRegularPolygonSource> polygon;
  polygon->SetGeneratePolyline(false);
  polygon->SetCenter(5, 5, 0);
  polygon->SetRadius(2);
  polygon->SetNumberOfSides(8);

  auto append = ::Append({ ::AddCellScalarArray("0.1", points),
    ::AddCellScalarArray("0.5", polyline), ::AddCellScalarArray("0.9", polygon) });

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(append->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(0.5, 0.0, 0.5, 1.0);
  ctf->AddRGBPoint(1.0, 0.0, 1.0, 0.0);
  mapper->SetLookupTable(ctf);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(10);
  actor->GetProperty()->SetPointSize(20);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(1, 1, 1);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return retVal == vtkTesting::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
}
