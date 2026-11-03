// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkPlotRadViz draws its input when it is set with
 * SetInputConnection, for a table and for a dataset, when the array to
 * process is not the active scalars, and when the active scalars become
 * unnamed. Check that it draws the arrays computed for cell scalars, that
 * column visibility set before the first paint is kept, that labels set with
 * SetLabels follow the visible columns, that SetPlot(nullptr) removes the
 * plot, and that it draws nothing, leaving no vertices to interact with, when
 * hidden or when too few columns remain after a paint.
 */

#include "vtkCellData.h"
#include "vtkChartRadViz.h"
#include "vtkContextMapper2D.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMultiVariateToRGBA.h"
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
#include <tuple>
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
 * Exposes the labels drawn at the vertices, and the arrays drawn.
 */
class InternalsPlot : public vtkPlotRadViz
{
public:
  static InternalsPlot* New();
  vtkTypeMacro(InternalsPlot, vtkPlotRadViz);
  vtkStringArray* GetVisibleLabels() { return this->VisibleLabels; }
  std::tuple<vtkDataArray*, vtkDataArray*, vtkDataArray*> GetDrawnArrays()
  {
    return this->GetVerticesGbcRgba(this->ComputeGBC->GetOutputDataObject(0));
  }
};
vtkStandardNewMacro(InternalsPlot);

/**
 * Returns an array named 'name' with 'numberOfComponents' components and
 * 'numberOfTuples' tuples.
 */
vtkSmartPointer<vtkDoubleArray> MakeArray(
  const char* name, int numberOfComponents, vtkIdType numberOfTuples = NumberOfValues)
{
  auto array = vtkSmartPointer<vtkDoubleArray>::New();
  array->SetName(name);
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfTuples);
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
  vtkNew<InternalsPlot> plot;
  chart->SetPlot(plot);
  plot->SetInputData(MakeTable());
  vtkNew<vtkStringArray> labelsArray;
  for (const std::string& label : labels)
  {
    labelsArray->InsertNextValue(label);
  }
  plot->SetLabels(labelsArray);
  plot->SetVariableVisibility("b", false);
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
 * Renders a RadViz chart of an image with 3 component cell scalars, and point
 * arrays named "gbc" and "rgba", and returns whether the plot drew the arrays
 * computed for the cells.
 */
bool PaintCellScalars()
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkNew<InternalsPlot> plot;
  chart->SetPlot(plot);
  vtkNew<vtkImageData> image;
  image->SetDimensions(5, 5, 2);
  image->GetCellData()->SetScalars(MakeArray("values", 3, image->GetNumberOfCells()));
  image->GetPointData()->AddArray(MakeArray("gbc", 2));
  image->GetPointData()->AddArray(MakeArray("rgba", 4));
  plot->SetInputDataSet(image);
  view->GetRenderWindow()->Render();
  auto [vertices, gbc, rgba] = plot->GetDrawnArrays();
  return vertices->GetNumberOfTuples() == 3 &&
    gbc->GetNumberOfTuples() == image->GetNumberOfCells() &&
    rgba->GetNumberOfTuples() == image->GetNumberOfCells();
}

/**
 * Renders a RadViz chart of a 4 column table, then changes the plot with
 * 'change' and renders it again, and returns the number of vertices left.
 */
vtkIdType RepaintVertices(const std::function<void(vtkPlotRadViz*)>& change)
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkPlotRadViz* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
  plot->SetInputData(MakeTable());
  view->GetRenderWindow()->Render();
  change(plot);
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
            plot->SetVariableVisibilityAll(false);
            for (const char* name : { "a", "b", "d" })
            {
              plot->SetVariableVisibility(name, true);
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
            plot->SetVisibleVariables(visible);
          }) == 3);
  // labels of the visible columns, or the column names for a label count
  // that does not match the columns
  CHECK(PaintLabels({ "A", "B", "C", "D" }, { "A", "C", "D" }));
  CHECK(PaintLabels({ "A", "B" }, { "a", "c", "d" }));
  // no plot after SetPlot(nullptr)
  {
    vtkNew<vtkContextView> view;
    vtkNew<vtkChartRadViz> chart;
    view->GetScene()->AddItem(chart);
    chart->AddPlot(0);
    chart->SetPlot(nullptr);
    CHECK(chart->GetNumberOfPlots() == 0 && chart->GetNumberOfItems() == 0);
    view->GetRenderWindow()->Render();
  }
  // the arrays computed for cell scalars, not point arrays of the same name
  CHECK(PaintCellScalars());
  // no vertices left after a plot draws nothing
  CHECK(RepaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetVariableVisibility("a", false);
            plot->SetVariableVisibility("b", false);
          }) == 0);
  CHECK(RepaintVertices([](vtkPlotRadViz* plot) { plot->SetVisible(false); }) == 0);
  // nothing for a hidden plot
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputData(MakeTable());
            plot->SetVisible(false);
          }) == 0);
  return EXIT_SUCCESS;
}
