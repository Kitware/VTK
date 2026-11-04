// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkPlotRadViz and vtkChartRadViz can be saved and restored:
 * a plot with a table input is saved without errors and restored with its
 * input, visibility and labels, and a restored chart has the saved plot.
 */

#include "vtkChartRadViz.h"
#include "vtkDeserializer.h"
#include "vtkDoubleArray.h"
#include "vtkInvoker.h"
#include "vtkMarshalContext.h"
#include "vtkNew.h"
#include "vtkPlotRadViz.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Line " << __LINE__ << ": check failed: " #condition << std::endl;                \
    return false;                                                                                  \
  }

using Names = std::vector<std::string>;

/**
 * Serializes 'source' and deserializes its state into a new object.
 * Returns nullptr if either half fails.
 */
template <typename T>
vtkSmartPointer<T> RoundTrip(T* source)
{
  vtkNew<vtkMarshalContext> context;
  vtkNew<vtkSerializer> serializer;
  vtkNew<vtkDeserializer> deserializer;
  vtkNew<vtkInvoker> invoker;
  serializer->SetContext(context);
  deserializer->SetContext(context);
  if (const char* error = nullptr;
      !context->CallRegistrars(serializer, deserializer, invoker, &error))
  {
    std::cerr << "Failed to call registrars: " << error << std::endl;
    return nullptr;
  }
  const auto result = serializer->SerializeJSON(source);
  auto idIt = result.find("Id");
  if (idIt == result.end() || !idIt->is_number_unsigned())
  {
    std::cerr << "No 'Id' in serialized state." << std::endl;
    return nullptr;
  }
  // a new id, so the deserializer builds a new object
  auto state = context->GetState(idIt->template get<vtkTypeUInt32>());
  const vtkTypeUInt32 newId = context->MakeId();
  state["Id"] = newId;
  if (!context->RegisterState(state))
  {
    std::cerr << "Failed to register state." << std::endl;
    return nullptr;
  }
  vtkSmartPointer<vtkObjectBase> restored;
  if (!deserializer->DeserializeJSON(newId, restored))
  {
    std::cerr << "DeserializeJSON failed." << std::endl;
    return nullptr;
  }
  return T::SafeDownCast(restored);
}

vtkSmartPointer<vtkTable> MakeTable()
{
  auto table = vtkSmartPointer<vtkTable>::New();
  for (const char* name : { "a", "b", "c", "d" })
  {
    vtkNew<vtkDoubleArray> column;
    column->SetName(name);
    column->SetNumberOfTuples(3);
    column->Fill(1.0);
    table->AddColumn(column);
  }
  return table;
}

Names ToNames(vtkStringArray* array)
{
  Names names;
  for (vtkIdType i = 0; array && i < array->GetNumberOfValues(); ++i)
  {
    names.push_back(array->GetValue(i));
  }
  return names;
}

bool TestPlot()
{
  // no labels set
  vtkNew<vtkPlotRadViz> plot;
  plot->SetInputData(MakeTable());
  plot->SetVariableVisibility("b", false);
  auto restored = RoundTrip(plot.Get());
  CHECK(restored);
  CHECK(restored->GetInput() && restored->GetInput()->GetNumberOfColumns() == 4);
  CHECK(ToNames(restored->GetVisibleVariables()) == Names({ "a", "c", "d" }));
  CHECK(restored->GetLabels() == nullptr);

  // labels set
  vtkNew<vtkStringArray> labels;
  for (const char* label : { "A", "B", "C", "D" })
  {
    labels->InsertNextValue(label);
  }
  plot->SetLabels(labels);
  restored = RoundTrip(plot.Get());
  CHECK(restored);
  CHECK(ToNames(restored->GetLabels()) == Names({ "A", "B", "C", "D" }));
  return true;
}

bool TestChart()
{
  vtkNew<vtkChartRadViz> chart;
  vtkPlot* plot = chart->AddPlot(0);
  auto restored = RoundTrip(chart.Get());
  CHECK(restored);
  CHECK(restored->GetNumberOfPlots() == 1);
  auto* restoredPlot = vtkPlotRadViz::SafeDownCast(restored->GetPlot(0));
  // the context already holds the plot, so it is restored as is, not the
  // plot a new chart starts with
  CHECK(restoredPlot == plot);
  CHECK(restored->GetNumberOfItems() == 1);
  return true;
}
}

int TestRadVizSerDes(int, char*[])
{
  if (!TestPlot())
  {
    std::cerr << "Plot failed" << std::endl;
    return EXIT_FAILURE;
  }
  if (!TestChart())
  {
    std::cerr << "Chart failed" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
