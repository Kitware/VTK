// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check the vtkMultiVariateToRGBA variable visibility, by name and by index,
 * for the columns of a table and the components of a dataset array: the
 * visible variables reported match the vertices computed, names and indices
 * do not mix until the visibility starts over, unnamed variables are named
 * after their index, and changes that do not change the visibility do not
 * modify the filter.
 */

#include <vtkCommand.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTestErrorObserver.h>

#include <algorithm>
#include <cstdlib>
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
    return false;                                                                                  \
  }

using Names = std::vector<std::string>;
using Indices = std::vector<int>;

constexpr int NumberOfVariables = 5;
constexpr vtkIdType NumberOfValues = 10;
const Names VariableNames = { "a", "b", "c", "d", "e" };

double Value(vtkIdType i, int variable)
{
  return static_cast<double>((i * (variable + 3) + variable) % 7);
}

vtkSmartPointer<vtkDataObject> MakeTable()
{
  vtkNew<vtkTable> table;
  for (int v = 0; v < NumberOfVariables; ++v)
  {
    vtkNew<vtkDoubleArray> column;
    column->SetName(VariableNames[v].c_str());
    column->SetNumberOfTuples(NumberOfValues);
    for (vtkIdType i = 0; i < NumberOfValues; ++i)
    {
      column->SetValue(i, Value(i, v));
    }
    table->AddColumn(column);
  }
  return table;
}

vtkSmartPointer<vtkDataObject> MakeImage(bool namedComponents)
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(static_cast<int>(NumberOfValues), 1, 1);
  vtkNew<vtkDoubleArray> values;
  values->SetName("mv");
  values->SetNumberOfComponents(NumberOfVariables);
  values->SetNumberOfTuples(NumberOfValues);
  for (int v = 0; v < NumberOfVariables; ++v)
  {
    if (namedComponents)
    {
      values->SetComponentName(v, VariableNames[v].c_str());
    }
    for (vtkIdType i = 0; i < NumberOfValues; ++i)
    {
      values->SetComponent(i, v, Value(i, v));
    }
  }
  image->GetPointData()->SetScalars(values);
  return image;
}

vtkSmartPointer<vtkMultiVariateToRGBA> MakeFilter(vtkDataObject* input)
{
  auto filter = vtkSmartPointer<vtkMultiVariateToRGBA>::New();
  filter->SetController(nullptr);
  filter->SetStoreVertices(true);
  filter->SetInputDataObject(0, input);
  return filter;
}

Names ToNames(vtkStringArray* array)
{
  Names names;
  for (vtkIdType i = 0; i < array->GetNumberOfValues(); ++i)
  {
    names.push_back(array->GetValue(i));
  }
  return names;
}

Names Select(const Names& names, const Indices& indices)
{
  Names selected;
  for (int index : indices)
  {
    selected.push_back(names[index]);
  }
  return selected;
}

// Returns the number of vertices computed, -1 if the filter failed.
vtkIdType NumberOfVertices(vtkMultiVariateToRGBA* filter)
{
  filter->Update();
  vtkDataArray* vertices = filter->GetOutputDataObject(0)->GetFieldData()->GetArray("vertices");
  return vertices ? vertices->GetNumberOfTuples() : -1;
}

// Checks the visible variables reported by name and index, and, for at
// least 3 of them, that they are the ones processed.
bool CheckVisible(vtkMultiVariateToRGBA* filter, const Names& names, const Indices& expected)
{
  CHECK(filter->GetVisibleVariableIndices() == expected);
  CHECK(ToNames(filter->GetVisibleVariables()) == Select(names, expected));
  for (int v = 0; v < NumberOfVariables; ++v)
  {
    bool visible = std::find(expected.begin(), expected.end(), v) != expected.end();
    CHECK(filter->GetVariableVisibility(v) == visible);
    CHECK(filter->GetVariableVisibility(names[v]) == visible);
  }
  if (expected.size() >= 3)
  {
    CHECK(NumberOfVertices(filter) == static_cast<vtkIdType>(expected.size()));
  }
  return true;
}

// Returns true if 'change' is reported as an error and leaves the filter
// unmodified.
bool Rejected(
  vtkMultiVariateToRGBA* filter, const std::function<void(vtkMultiVariateToRGBA*)>& change)
{
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  unsigned long tag = filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  vtkMTimeType mtime = filter->GetMTime();
  change(filter);
  filter->RemoveObserver(tag);
  return errorObserver->GetError() && filter->GetMTime() == mtime;
}

// Runs the variable visibility checks on the input made by 'makeInput',
// whose 5 variables are named 'names': visibility by index and by name,
// names and indices not mixing, changes that do not modify the filter, and
// visibility set before the input is connected. Returns true if they pass.
bool TestInput(const std::function<vtkSmartPointer<vtkDataObject>()>& makeInput, const Names& names)
{
  vtkSmartPointer<vtkDataObject> input = makeInput();
  vtkSmartPointer<vtkMultiVariateToRGBA> filter = MakeFilter(input);
  CHECK(ToNames(filter->GetAllVariableNames()) == names);
  CHECK(CheckVisible(filter, names, { 0, 1, 2, 3, 4 }));

  // by index
  filter->AddVisibleVariableIndex(0);
  filter->AddVisibleVariableIndex(1);
  filter->AddVisibleVariableIndex(4);
  CHECK(CheckVisible(filter, names, { 0, 1, 4 }));
  filter->ResetVisibleVariableIndices();
  CHECK(CheckVisible(filter, names, { 0, 1, 2, 3, 4 }));
  // indices outside the input are ignored
  for (int index : { 0, 2, 3, 9 })
  {
    filter->AddVisibleVariableIndex(index);
  }
  CHECK(CheckVisible(filter, names, { 0, 2, 3 }));
  filter->SetVisibleVariableIndices({ 0, 2, 4, 9 });
  CHECK(CheckVisible(filter, names, { 0, 2, 4 }));
  filter->SetVariableVisibility(1, true);
  CHECK(CheckVisible(filter, names, { 0, 1, 2, 4 }));
  filter->SetVisibleVariableIndices({});
  CHECK(CheckVisible(filter, names, {}));
  CHECK(
    Rejected(filter, [](vtkMultiVariateToRGBA* f) { f->SetVisibleVariableIndices({ 0, -1 }); }));
  CHECK(Rejected(filter, [](vtkMultiVariateToRGBA* f) { f->SetVariableVisibility(-1, true); }));
  CHECK(Rejected(filter, [](vtkMultiVariateToRGBA* f) { f->AddVisibleVariableIndex(-1); }));

  // by name
  filter->SetVariableVisibilityAll(true);
  filter->SetVariableVisibility(names[1], false);
  CHECK(CheckVisible(filter, names, { 0, 2, 3, 4 }));
  vtkNew<vtkStringArray> visible;
  for (int index : { 0, 2, 4 })
  {
    visible->InsertNextValue(names[index]);
  }
  filter->SetVisibleVariables(visible);
  CHECK(CheckVisible(filter, names, { 0, 2, 4 }));
  filter->SetVariableVisibilityAll(false);
  CHECK(CheckVisible(filter, names, {}));
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  unsigned long filterTag = filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  unsigned long executiveTag =
    filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  CHECK(NumberOfVertices(filter) == -1);
  CHECK(errorObserver->GetError());
  filter->RemoveObserver(filterTag);
  filter->GetExecutive()->RemoveObserver(executiveTag);

  // names and indices do not mix until the visibility starts over
  filter->SetVariableVisibilityAll(true);
  filter->SetVariableVisibility(names[1], false);
  const std::string name3 = names[3];
  CHECK(Rejected(filter, [](vtkMultiVariateToRGBA* f) { f->SetVariableVisibility(1, true); }));
  CHECK(Rejected(filter, [](vtkMultiVariateToRGBA* f) { f->AddVisibleVariableIndex(3); }));
  CHECK(CheckVisible(filter, names, { 0, 2, 3, 4 }));
  filter->SetVisibleVariableIndices({ 0, 1, 2 });
  CHECK(Rejected(
    filter, [&name3](vtkMultiVariateToRGBA* f) { f->SetVariableVisibility(name3, true); }));
  CHECK(CheckVisible(filter, names, { 0, 1, 2 }));
  filter->SetVariableVisibilityAll(true);
  filter->SetVariableVisibility(names[3], false);
  CHECK(CheckVisible(filter, names, { 0, 1, 2, 4 }));
  filter->SetVisibleVariables(visible);
  CHECK(Rejected(filter, [](vtkMultiVariateToRGBA* f) { f->SetVariableVisibility(1, true); }));
  // a change to the default stores nothing, so it does not set the kind
  filter->SetVariableVisibilityAll(true);
  filter->SetVariableVisibility(names[1], true);
  filter->SetVariableVisibility(1, false);
  CHECK(CheckVisible(filter, names, { 0, 2, 3, 4 }));

  // changes that do not change the visibility do not modify the filter
  filter->SetVariableVisibilityAll(true);
  filter->SetVariableVisibility(names[1], false);
  vtkMTimeType mtime = filter->GetMTime();
  filter->SetVariableVisibility(names[1], false);
  filter->SetVariableVisibility(names[2], true);
  CHECK(filter->GetMTime() == mtime);
  filter->ResetVisibleVariableIndices();
  filter->AddVisibleVariableIndex(0);
  filter->AddVisibleVariableIndex(1);
  filter->AddVisibleVariableIndex(2);
  mtime = filter->GetMTime();
  filter->AddVisibleVariableIndex(1);
  filter->SetVariableVisibility(4, false);
  CHECK(filter->GetMTime() == mtime);
  filter->ResetVisibleVariableIndices();
  mtime = filter->GetMTime();
  filter->SetVariableVisibilityAll(true);
  filter->ResetVisibleVariableIndices();
  CHECK(filter->GetMTime() == mtime);

  // visibility set before the input is connected
  vtkNew<vtkMultiVariateToRGBA> before;
  before->SetController(nullptr);
  before->SetStoreVertices(true);
  before->AddVisibleVariableIndex(1);
  before->AddVisibleVariableIndex(2);
  before->SetVariableVisibility(3, true);
  before->SetInputDataObject(0, input);
  CHECK(CheckVisible(before, names, { 1, 2, 3 }));
  return true;
}
}

int TestMultiVariateToRGBAVisibleVariables(int, char*[])
{
  // variables are the columns a to e of a table
  if (!TestInput(MakeTable, VariableNames))
  {
    std::cerr << "Table failed" << std::endl;
    return EXIT_FAILURE;
  }
  // variables are the components, named a to e, of the image scalars
  if (!TestInput([] { return MakeImage(true); }, VariableNames))
  {
    std::cerr << "Image with named components failed" << std::endl;
    return EXIT_FAILURE;
  }
  // the same, unnamed: the components are named after their index
  if (!TestInput([] { return MakeImage(false); }, { "0", "1", "2", "3", "4" }))
  {
    std::cerr << "Image with unnamed components failed" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
