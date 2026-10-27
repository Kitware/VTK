// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkPlotRadViz draws its input when it is set with
 * SetInputConnection, for a table and for a dataset, and when the array to
 * process is not the active scalars, when the active scalars become
 * unnamed, that column visibility set before the
 * first paint is kept, that labels set with SetLabels follow the visible
 * columns, and that it draws nothing when hidden.
 */

#include "vtkChartRadViz.h"
#include "vtkContextMapper2D.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPassThrough.h"
#include "vtkPlotRadViz.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTrivialProducer.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

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
 * Exposes the labels drawn at the vertices.
 */
class LabelsPlot : public vtkPlotRadViz
{
public:
  static LabelsPlot* New();
  vtkTypeMacro(LabelsPlot, vtkPlotRadViz);
  vtkStringArray* GetVisibleLabels() { return this->VisibleLabels; }
};
vtkStandardNewMacro(LabelsPlot);

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
 * Renders a RadViz chart of an image whose active scalars are named, then of
 * an image whose active scalars are unnamed with 4 components, and returns
 * the number of vertices the plot drew for the second image.
 */
vtkIdType PaintUnnamedScalars()
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
  plot->SetInputDataSet(MakeImage(false));
  view->GetRenderWindow()->Render();
  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(5, 5, 2);
  image->GetPointData()->SetScalars(MakeArray(nullptr, 4));
  plot->SetInputDataSet(image);
  view->GetRenderWindow()->Render();
  vtkPoints2D* vertices = plot->GetVertices();
  return vertices ? vertices->GetNumberOfPoints() : 0;
}

/**
 * Renders a RadViz chart of a 4 column table, with 'labels' set and column
 * "b" hidden, and returns whether the plot drew 'expected' as vertex labels.
 */
bool PaintLabels(const std::vector<std::string>& labels, const std::vector<std::string>& expected)
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkNew<LabelsPlot> plot;
  chart->SetPlot(plot);
  plot->SetInputData(MakeTable());
  vtkNew<vtkStringArray> labelsArray;
  for (const std::string& label : labels)
  {
    labelsArray->InsertNextValue(label);
  }
  plot->SetLabels(labelsArray);
  plot->SetColumnVisibility("b", false);
  view->GetRenderWindow()->Render();
  vtkStringArray* visibleLabels = plot->GetVisibleLabels();
  if (!visibleLabels ||
    visibleLabels->GetNumberOfValues() != static_cast<vtkIdType>(expected.size()))
  {
    return false;
  }
  for (size_t i = 0; i < expected.size(); ++i)
  {
    if (visibleLabels->GetValue(i) != expected[i])
    {
      return false;
    }
  }
  return true;
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
  CHECK(
    PaintVertices([](vtkPlotRadViz* plot) { SetInputConnection(plot, MakeImage(false)); }) == 3);
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputDataSet(MakeImage(true));
            plot->GetData()->SetInputArrayToProcess(
              0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "values");
          }) == 3);
  // the unnamed active scalars, not the array processed before
  CHECK(PaintUnnamedScalars() == 4);
  // visibility set before the first paint
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            SetInputConnection(plot, MakeTable());
            plot->SetColumnVisibilityAll(false);
            for (const char* name : { "a", "b", "d" })
            {
              plot->SetColumnVisibility(name, true);
            }
          }) == 3);
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            SetInputConnection(plot, MakeTable());
            vtkNew<vtkStringArray> visible;
            for (const char* name : { "a", "c", "d" })
            {
              visible->InsertNextValue(name);
            }
            plot->SetVisibleColumns(visible);
          }) == 3);
  // labels of the visible columns, or the column names for a label count
  // that does not match the columns
  CHECK(PaintLabels({ "A", "B", "C", "D" }, { "A", "C", "D" }));
  CHECK(PaintLabels({ "A", "B" }, { "a", "c", "d" }));
  // nothing for a hidden plot
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputData(MakeTable());
            plot->SetVisible(false);
          }) == 0);
  return EXIT_SUCCESS;
}
