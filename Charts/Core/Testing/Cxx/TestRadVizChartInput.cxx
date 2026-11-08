// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkPlotRadViz draws its input when it is set with
 * SetInputConnection, for a table and for a dataset, when the array to
 * process is not the active scalars, and when the active scalars become
 * unnamed. Check that it draws the arrays computed for cell scalars, that
 * variable visibility, by name or index, set before the first paint is kept,
 * that labels set with SetLabels follow the visible variables, that
 * SetPlot(nullptr) removes the plot, that AddPlot(vtkPlot*), RemovePlot()
 * and ClearPlots() set or remove a RadViz plot only, that GetLabels()
 * returns only the labels set, and that it draws nothing, leaving no
 * vertices to interact with, when hidden or when too few variables remain
 * after a paint. Check that unchanged or rejected visibility settings do not
 * modify the plot, that the partition and custom range reach the filter
 * when set, that a table column with several components is
 * reported as an error, that a string column is hidden by default, that
 * SetInputArray selects a point or cell array of a dataset, that the
 * visibility getters follow the input and array selection before a paint,
 * that the getters and painting again without changes do not modify the
 * filter, that it draws nothing when the labels leave no room, that
 * drawing the labels leaves LabelsProperties unchanged, and that the mouse
 * finds a vertex or the lens outline anywhere on their drawn width.
 */

#include "vtkCellData.h"
#include "vtkChartRadViz.h"
#include "vtkCommand.h"
#include "vtkContextMapper2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMultiVariateToRGBA.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPassThrough.h"
#include "vtkPen.h"
#include "vtkPlotPoints.h"
#include "vtkPlotRadViz.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderWindow.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTestErrorObserver.h"
#include "vtkTextProperty.h"
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
  vtkMultiVariateToRGBA* GetFilter() { return this->ComputeGBC; }
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
 * "b" hidden, by name or by index, and returns whether the plot drew
 * 'expected' as vertex labels.
 */
bool PaintLabels(const std::vector<std::string>& labels, const std::vector<std::string>& expected,
  bool byIndex = false)
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
  if (byIndex)
  {
    for (int index : { 0, 2, 3 })
    {
      plot->AddVisibleVariableIndex(index);
    }
  }
  else
  {
    plot->SetVariableVisibility("b", false);
  }
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
 * Renders a RadViz chart of an image whose active scalars are unnamed with 4
 * components, the second one hidden by index, and returns whether the plot
 * drew 3 vertices labeled with the indices of the other components.
 */
bool PaintImageHiddenByIndex()
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkNew<InternalsPlot> plot;
  chart->SetPlot(plot);
  vtkNew<vtkImageData> image;
  image->SetDimensions(5, 5, 2);
  image->GetPointData()->SetScalars(MakeArray(nullptr, 4));
  plot->SetInputDataSet(image);
  plot->SetVariableVisibility(1, false);
  view->GetRenderWindow()->Render();
  vtkPoints2D* vertices = plot->GetVertices();
  vtkStringArray* labels = plot->GetVisibleLabels();
  return vertices && vertices->GetNumberOfPoints() == 3 && labels &&
    labels->GetNumberOfValues() == 3 && labels->GetValue(0) == "0" && labels->GetValue(1) == "2" &&
    labels->GetValue(2) == "3";
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

/**
 * Checks that the visibility setters modify the plot only when they change
 * the visibility, not when it is unchanged or the change is rejected.
 */
int TestPlotModified()
{
  vtkNew<InternalsPlot> plot;
  plot->SetInputData(MakeTable());
  vtkNew<vtkTest::ErrorObserver> errors;
  plot->GetFilter()->AddObserver(vtkCommand::ErrorEvent, errors);
  auto modifies = [&plot](const std::function<void()>& change)
  {
    vtkMTimeType mtime = plot->GetMTime();
    change();
    return plot->GetMTime() != mtime;
  };
  vtkNew<vtkStringArray> bcd;
  for (const char* name : { "b", "c", "d" })
  {
    bcd->InsertNextValue(name);
  }

  CHECK(!modifies([&] { plot->SetVariableVisibilityAll(true); }));
  CHECK(!modifies([&] { plot->SetVariableVisibility("a", true); }));
  CHECK(modifies([&] { plot->SetVariableVisibility("a", false); }));
  CHECK(!modifies([&] { plot->SetVariableVisibility("a", false); }));
  CHECK(modifies([&] { plot->SetVisibleVariables(bcd); }));
  CHECK(!modifies([&] { plot->SetVisibleVariables(bcd); }));
  // rejected: the visibility is set by name
  CHECK(!modifies([&] { plot->SetVariableVisibility(1, false); }));
  CHECK(errors->GetError());
  errors->Clear();

  CHECK(modifies([&] { plot->ResetVisibleVariableIndices(); }));
  CHECK(!modifies([&] { plot->ResetVisibleVariableIndices(); }));
  CHECK(modifies([&] { plot->AddVisibleVariableIndex(2); }));
  CHECK(!modifies([&] { plot->SetVisibleVariableIndices({ 2 }); }));
  CHECK(modifies([&] { plot->SetVisibleVariableIndices({ 0, 2 }); }));
  // rejected: a negative index
  CHECK(!modifies([&] { plot->AddVisibleVariableIndex(-1); }));
  CHECK(errors->GetError());

  // the partition and custom range are set on the filter
  CHECK(modifies([&] { plot->SetPartitionIndices({ 0, 1, 2 }); }));
  CHECK(!modifies([&] { plot->SetPartitionIndices({ 0, 1, 2 }); }));
  CHECK(modifies([&] { plot->SetPartitionOffsets({ 1, 3 }); }));
  CHECK(modifies([&] { plot->SetCustomRange({ 0, 1, 0, 2 }); }));
  CHECK(!modifies([&] { plot->SetCustomRange({ 0, 1, 0, 2 }); }));
  CHECK(plot->GetFilter()->GetPartitionIndices() == std::vector<int>({ 0, 1, 2 }));
  CHECK(plot->GetFilter()->GetPartitionOffsets() == std::vector<int>({ 1, 3 }));
  CHECK(plot->GetFilter()->GetCustomRange() == std::vector<double>({ 0, 1, 0, 2 }));
  CHECK(modifies([&] { plot->ClearPartitionIndices(); }));
  CHECK(!modifies([&] { plot->ClearPartitionIndices(); }));
  CHECK(modifies([&] { plot->AddPartitionIndex(2); }));
  CHECK(modifies([&] { plot->ClearPartitionOffsets(); }));
  CHECK(modifies([&] { plot->AddPartitionOffset(1); }));
  CHECK(plot->GetPartitionIndices() == std::vector<int>({ 2 }));
  CHECK(plot->GetFilter()->GetPartitionIndices() == std::vector<int>({ 2 }));
  CHECK(plot->GetFilter()->GetPartitionOffsets() == std::vector<int>({ 1 }));
  return EXIT_SUCCESS;
}
/**
 * Checks that the visibility getters follow the input connection and the
 * array selection without a paint, and that they and a paint without
 * changes do not modify the filter.
 */
int TestFilterInput()
{
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(300, 300);
  vtkNew<vtkChartRadViz> chart;
  view->GetScene()->AddItem(chart);
  vtkNew<InternalsPlot> plot;
  chart->SetPlot(plot);
  vtkNew<vtkTrivialProducer> producer;
  producer->SetOutput(MakeTable());
  plot->SetInputConnection(producer->GetOutputPort());
  CHECK(plot->GetVisibleVariables()->GetNumberOfValues() == 4);
  CHECK(plot->GetVisibleVariableIndices().size() == 4);
  CHECK(plot->GetVariableVisibility("d"));
  CHECK(plot->GetVariableVisibility(3));

  // the active scalars have one component, "values" three
  plot->SetInputDataSet(MakeImage(true));
  CHECK(plot->GetVisibleVariables()->GetNumberOfValues() == 1);
  plot->SetInputArray(0, "values");
  CHECK(plot->GetVisibleVariables()->GetNumberOfValues() == 3);
  view->GetRenderWindow()->Render();
  CHECK(plot->GetVertices() && plot->GetVertices()->GetNumberOfPoints() == 3);
  plot->GetData()->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  CHECK(plot->GetVisibleVariableIndices().size() == 1);
  CHECK(!plot->GetVariableVisibility(1));

  // painting again without changes
  plot->SetInputArray(0, "values");
  view->GetRenderWindow()->Render();
  vtkMTimeType mtime = plot->GetFilter()->GetMTime();
  view->GetRenderWindow()->Render();
  plot->GetVisibleVariables();
  plot->GetVariableVisibility(1);
  CHECK(plot->GetFilter()->GetMTime() == mtime);
  return EXIT_SUCCESS;
}

/**
 * Checks that the mouse finds a vertex, or the lens outline, 8 pixels away
 * from its center line when its pen is 20 pixels wide, and not with the
 * default pen.
 */
int TestHitArea()
{
  for (bool wide : { true, false })
  {
    vtkNew<vtkContextView> view;
    view->GetRenderWindow()->SetSize(300, 300);
    vtkNew<vtkChartRadViz> chart;
    view->GetScene()->AddItem(chart);
    auto* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
    plot->SetInputData(MakeTable());
    plot->SetLensVisibility(true);
    if (wide)
    {
      plot->GetVerticesPen()->SetWidth(20);
      plot->GetLensPen()->SetWidth(20);
    }
    view->GetRenderWindow()->Render();
    vtkContextMouseEvent mouse;
    mouse.SetButton(vtkContextMouseEvent::NO_BUTTON);

    double vertex[2];
    plot->GetVertices()->GetPoint(0, vertex);
    mouse.SetScenePos(vtkVector2f(vertex[0] + 8, vertex[1]));
    chart->MouseMoveEvent(mouse);
    CHECK((plot->GetActiveVertexIndex() == 0) == wide);

    const double* lensCenter = plot->GetSceneLensCenter();
    double lensRadius = plot->GetSceneLensRadius();
    // at 45 degrees, away from the 4 vertices the outline may pass through
    double offset = (lensRadius + 8) / std::sqrt(2.0);
    mouse.SetScenePos(vtkVector2f(lensCenter[0] + offset, lensCenter[1] + offset));
    chart->MouseMoveEvent(mouse);
    CHECK(plot->GetActiveVertexIndex() == -1);
    CHECK(plot->GetActiveLensOutline() == wide);
  }
  return EXIT_SUCCESS;
}

int TestRadVizChartInput(int, char*[])
{
  CHECK(TestPlotModified() == EXIT_SUCCESS);
  CHECK(TestFilterInput() == EXIT_SUCCESS);
  CHECK(TestHitArea() == EXIT_SUCCESS);
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
  // SetInputArray selects a point array, or a cell array, of a dataset
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputDataSet(MakeImage(true));
            plot->SetInputArray(0, "values");
          }) == 3);
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            vtkSmartPointer<vtkImageData> image = MakeImage(true);
            image->GetCellData()->AddArray(MakeArray("cellValues", 4, image->GetNumberOfCells()));
            plot->SetInputDataSet(image);
            plot->SetInputArray(0, "cellValues");
          }) == 4);
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
  CHECK(PaintLabels({ "A", "B", "C", "D" }, { "A", "C", "D" }, true));
  CHECK(PaintLabels({ "A", "B" }, { "a", "c", "d" }, true));
  // visibility by index, for a table and a dataset
  // 4 columns, "c" hidden by index: 3 vertices (PaintLabels checks which)
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            SetInputConnection(plot, MakeTable());
            plot->SetVariableVisibility(2, false);
          }) == 3);
  // 3 components, one hidden by index: too few to draw (3 if the index were ignored)
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            SetInputConnection(plot, MakeImage(false));
            plot->SetVariableVisibility(1, false);
          }) == 0);
  // 4 components, the second hidden by index: the other 3 drawn
  CHECK(PaintImageHiddenByIndex());
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
  // AddPlot(vtkPlot*) replaces the plot, ignores other plot types
  {
    vtkNew<vtkChartRadViz> chart;
    vtkPlot* first = chart->AddPlot(0);
    vtkNew<vtkPlotPoints> points;
    CHECK(chart->AddPlot(points) == -1);
    CHECK(chart->GetPlot(0) == first && chart->GetNumberOfItems() == 1);
    vtkNew<vtkPlotRadViz> second;
    CHECK(chart->AddPlot(second) == 0);
    CHECK(chart->GetPlot(0) == second && chart->GetNumberOfItems() == 1);
    CHECK(!chart->RemovePlot(1));
    CHECK(chart->RemovePlot(0));
    CHECK(chart->GetNumberOfPlots() == 0 && chart->GetNumberOfItems() == 0);
    CHECK(!chart->RemovePlot(0));
    chart->AddPlot(second);
    chart->ClearPlots();
    CHECK(chart->GetNumberOfPlots() == 0 && chart->GetNumberOfItems() == 0);
  }
  // GetLabels() returns only the labels set, not the ones drawn
  {
    vtkNew<vtkContextView> view;
    vtkNew<vtkChartRadViz> chart;
    view->GetScene()->AddItem(chart);
    auto* plot = vtkPlotRadViz::SafeDownCast(chart->AddPlot(0));
    plot->SetInputData(MakeTable());
    view->GetRenderWindow()->Render();
    CHECK(plot->GetLabels() == nullptr);
    vtkNew<vtkStringArray> labels;
    labels->InsertNextValue("A");
    plot->SetLabels(labels);
    CHECK(plot->GetLabels() == labels.Get());
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
  // a table column with several components, alone or with other columns
  {
    const std::string several = "Column has several components: multi";
    vtkNew<vtkTest::ErrorObserver> errors;
    for (bool alone : { true, false })
    {
      auto table = vtkSmartPointer<vtkTable>::New();
      if (!alone)
      {
        table->AddColumn(MakeArray("a", 1));
        table->AddColumn(MakeArray("b", 1));
      }
      table->AddColumn(MakeArray("multi", 4));
      errors->Clear();
      CHECK(PaintVertices(
              [&](vtkPlotRadViz* plot)
              {
                plot->AddObserver(vtkCommand::ErrorEvent, errors);
                plot->SetInputData(table);
              }) == 0);
      CHECK(errors->HasErrorMessage(several));
    }
  }
  // a string column is hidden by default
  {
    vtkNew<vtkTest::ErrorObserver> errors;
    CHECK(PaintVertices(
            [&](vtkPlotRadViz* plot)
            {
              plot->AddObserver(vtkCommand::ErrorEvent, errors);
              vtkSmartPointer<vtkTable> table = MakeTable();
              vtkNew<vtkStringArray> species;
              species->SetName("species");
              species->SetNumberOfValues(NumberOfValues);
              table->AddColumn(species);
              plot->SetInputData(table);
            }) == 4);
    CHECK(!errors->GetError());
  }
  // nothing when the labels leave no room for the plot
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputData(MakeTable());
            vtkNew<vtkStringArray> labels;
            for (int i = 0; i < 4; ++i)
            {
              labels->InsertNextValue(std::string(40, 'W'));
            }
            plot->SetLabels(labels);
          }) == 0);
  // drawing the labels leaves LabelsProperties unchanged
  {
    vtkNew<vtkTextProperty> labelsProperties;
    labelsProperties->SetJustificationToRight();
    labelsProperties->SetVerticalJustificationToTop();
    vtkMTimeType mtime = labelsProperties->GetMTime();
    CHECK(PaintVertices(
            [&](vtkPlotRadViz* plot)
            {
              plot->SetInputData(MakeTable());
              plot->SetLabelsProperties(labelsProperties);
            }) == 4);
    CHECK(labelsProperties->GetJustification() == VTK_TEXT_RIGHT);
    CHECK(labelsProperties->GetVerticalJustification() == VTK_TEXT_TOP);
    CHECK(labelsProperties->GetMTime() == mtime);
  }
  // nothing for a hidden plot
  CHECK(PaintVertices(
          [](vtkPlotRadViz* plot)
          {
            plot->SetInputData(MakeTable());
            plot->SetVisible(false);
          }) == 0);
  return EXIT_SUCCESS;
}
