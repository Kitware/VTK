// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkPlotRadViz draws its input when it is set with
 * SetInputConnection, and when the array to process is not the active scalars.
 */

#include "vtkChartRadViz.h"
#include "vtkContextMapper2D.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPassThrough.h"
#include "vtkPlotRadViz.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindow.h"
#include "vtkTable.h"
#include "vtkTrivialProducer.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <string>

namespace
{
#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Line " << __LINE__ << ": check failed: " #condition << std::endl;                \
    return EXIT_FAILURE;                                                                           \
  }

constexpr vtkIdType NumberOfValues = 50;

/**
 * Returns an array named 'name' with 'numberOfComponents' components.
 */
vtkSmartPointer<vtkDoubleArray> MakeArray(const char* name, int numberOfComponents)
{
  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetName(name);
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(NumberOfValues);
  for (vtkIdType i = 0; i < array->GetNumberOfValues(); ++i)
  {
    array->SetValue(i, std::fabs(std::sin(0.37 * i)));
  }
  return array;
}

/**
 * Returns a table with 4 columns.
 */
vtkSmartPointer<vtkTable> MakeTable()
{
  auto table = vtkSmartPointer<vtkTable>::New();
  for (const char* name : { "a", "b", "c", "d" })
  {
    table->AddColumn(MakeArray(name, 1));
  }
  return table;
}

/**
 * Returns an image with 50 points and a 3 component array, the active
 * scalars unless 'singleActive' is true: then the active scalars have one
 * component.
 */
vtkSmartPointer<vtkImageData> MakeImage(bool singleActive)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(5, 5, 2);
  image->GetPointData()->AddArray(MakeArray("values", 3));
  if (singleActive)
  {
    image->GetPointData()->SetScalars(MakeArray("single", 1));
  }
  else
  {
    image->GetPointData()->SetActiveScalars("values");
  }
  return image;
}

/**
 * Renders a RadViz chart with a plot set up by 'setInput' and returns the
 * number of vertices the plot drew.
 */
vtkIdType PaintVertices(const std::function<void(vtkPlotRadViz*)>& setInput)
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
  setInput(plot);
  view->GetRenderWindow()->Render();
  vtkPoints2D* vertices = plot->GetVertices();
  return vertices ? vertices->GetNumberOfPoints() : 0;
}

/**
 * Connects the plot to a pipeline that has not been updated, with 'data'
 * at its start.
 */
void SetInputConnection(vtkPlotRadViz* plot, vtkDataObject* data)
{
  vtkNew<vtkTrivialProducer> producer;
  producer->SetOutput(data);
  vtkNew<vtkPassThrough> passThrough;
  passThrough->SetInputConnection(producer->GetOutputPort());
  plot->SetInputConnection(passThrough->GetOutputPort());
}
}

int TestRadVizChartInput(int, char*[])
{
  // a column per vertex
  CHECK(PaintVertices([](vtkPlotRadViz* plot) { SetInputConnection(plot, MakeTable()); }) == 4);
  // a component per vertex
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputDataSet(MakeImage(true));
            plot->GetData()->SetInputArrayToProcess(
              0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "values");
          }) == 3);
  return EXIT_SUCCESS;
}
