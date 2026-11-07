// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check vtkMultiVariateToRGBA on degenerate input: constant components and
 * groups, single component groups, integer types at their limits, a single
 * point, no points, fewer than 3 components and no input. The output has no
 * NaNs and matches the expected normalization, or the filter fails without
 * crashing. Also check that invalid partitions, custom ranges and output
 * selections are reported as errors.
 */

#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataArrayRange.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTestErrorObserver.h>

#include <algorithm>
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

using Rows = std::vector<std::vector<double>>;
constexpr double Tolerance = 1e-6;

// Returns an image with one point per row and the point array 'mv' of type
// 'dataType' holding the rows.
vtkSmartPointer<vtkImageData> MakeImage(const Rows& rows, int dataType = VTK_DOUBLE)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(static_cast<int>(rows.size()), 1, 1);
  auto values = vtkSmartPointer<vtkDataArray>::Take(vtkDataArray::CreateDataArray(dataType));
  values->SetName("mv");
  values->SetNumberOfComponents(static_cast<int>(rows[0].size()));
  values->SetNumberOfTuples(static_cast<vtkIdType>(rows.size()));
  for (size_t i = 0; i < rows.size(); ++i)
  {
    for (size_t c = 0; c < rows[i].size(); ++c)
    {
      values->SetComponent(static_cast<vtkIdType>(i), static_cast<int>(c), rows[i][c]);
    }
  }
  image->GetPointData()->AddArray(values);
  return image;
}

// Runs the filter on 'mv' of 'input', after 'setup', with expected errors
// silenced or caught by 'errorObserver'. Returns the filter.
vtkSmartPointer<vtkMultiVariateToRGBA> Run(
  vtkDataObject* input,
  const std::function<void(vtkMultiVariateToRGBA*)>& setup = [](vtkMultiVariateToRGBA*) {},
  vtkTest::ErrorObserver* errorObserver = nullptr)
{
  auto filter = vtkSmartPointer<vtkMultiVariateToRGBA>::New();
  vtkNew<vtkTest::ErrorObserver> silencer;
  if (!errorObserver)
  {
    errorObserver = silencer;
  }
  filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->SetController(nullptr);
  filter->SetInputData(input);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "mv");
  filter->SetStoreGBC(true);
  setup(filter);
  filter->Update();
  return filter;
}

vtkDataArray* GetArray(vtkMultiVariateToRGBA* filter, const char* name)
{
  auto* output = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));
  return output ? output->GetPointData()->GetArray(name) : nullptr;
}

// Returns true if the rgba and gbc arrays exist and have no NaNs.
bool Finite(vtkMultiVariateToRGBA* filter)
{
  for (const char* name : { "rgba", "gbc" })
  {
    vtkDataArray* array = GetArray(filter, name);
    if (!array)
    {
      return false;
    }
    auto values = vtk::DataArrayValueRange(array);
    if (!std::all_of(values.begin(), values.end(), [](double v) { return std::isfinite(v); }))
    {
      return false;
    }
  }
  return true;
}

bool Equal(vtkDataArray* a, vtkDataArray* b)
{
  auto aValues = vtk::DataArrayValueRange(a);
  auto bValues = vtk::DataArrayValueRange(b);
  return aValues.size() == bValues.size() &&
    std::equal(aValues.begin(), aValues.end(), bValues.begin(),
      [](double x, double y) { return std::fabs(x - y) <= Tolerance; });
}

// Returns true if the alpha of the rgba array is 'expected'.
bool AlphaIs(vtkMultiVariateToRGBA* filter, const std::vector<double>& expected)
{
  vtkDataArray* rgba = GetArray(filter, "rgba");
  if (!rgba || rgba->GetNumberOfTuples() != static_cast<vtkIdType>(expected.size()))
  {
    return false;
  }
  for (size_t i = 0; i < expected.size(); ++i)
  {
    if (std::fabs(rgba->GetComponent(static_cast<vtkIdType>(i), 3) - expected[i]) > Tolerance)
    {
      std::cerr << "alpha " << i << ": " << rgba->GetComponent(static_cast<vtkIdType>(i), 3)
                << ", expected " << expected[i] << std::endl;
      return false;
    }
  }
  return true;
}

// Returns true if every gbc is at the center.
bool AtCenter(vtkMultiVariateToRGBA* filter)
{
  auto values = vtk::DataArrayValueRange(GetArray(filter, "gbc"));
  return std::all_of(
    values.begin(), values.end(), [](double v) { return std::fabs(v) <= Tolerance; });
}

// Alpha is the mean of the normalized components, for the default lens.
std::vector<double> MeanAlpha(const Rows& normalized)
{
  std::vector<double> alpha;
  for (const auto& row : normalized)
  {
    double sum = 0;
    for (double value : row)
    {
      sum += value;
    }
    alpha.push_back(sum / row.size());
  }
  return alpha;
}

void SetGroups(vtkMultiVariateToRGBA* filter, const std::vector<int>& offsets)
{
  for (int i = 0; i < offsets.back(); ++i)
  {
    filter->AddPartitionIndex(i);
  }
  for (int offset : offsets)
  {
    filter->AddPartitionOffset(offset);
  }
}

using Setup = std::function<void(vtkMultiVariateToRGBA*)>;

// Returns true if the filter, after 'setup', reports an error containing
// 'message' and adds no arrays.
bool Rejected(vtkDataObject* input, const Setup& setup, const std::string& message)
{
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  auto filter = Run(input, setup, errorObserver);
  return errorObserver->HasErrorMessage(message) && !GetArray(filter, "rgba") &&
    !GetArray(filter, "gbc");
}

// Returns a setup with the partition 'indices' and 'offsets'.
Setup Partition(const std::vector<int>& indices, const std::vector<int>& offsets)
{
  return [=](vtkMultiVariateToRGBA* f)
  {
    f->SetPartitionIndices(indices);
    f->SetPartitionOffsets(offsets);
  };
}
}

int TestMultiVariateToRGBADegenerate(int, char*[])
{
  // A constant array: every component normalizes to 0, so the points are at
  // the center, transparent and gray.
  auto filter = Run(MakeImage(Rows(4, { 7, 7, 7, 7 })));
  CHECK(Finite(filter));
  CHECK(AtCenter(filter));
  CHECK(AlphaIs(filter, { 0, 0, 0, 0 }));
  vtkDataArray* rgba = GetArray(filter, "rgba");
  for (vtkIdType i = 0; i < rgba->GetNumberOfTuples(); ++i)
  {
    CHECK(rgba->GetComponent(i, 0) == rgba->GetComponent(i, 1));
    CHECK(rgba->GetComponent(i, 1) == rgba->GetComponent(i, 2));
  }

  // The last component is constant. In a group of its own it normalizes to 0;
  // the others are normalized over their shared range, [0, 4].
  const Rows oneConstant = { { 0, 1, 2, 7 }, { 1, 3, 0, 7 }, { 2, 0, 4, 7 }, { 4, 2, 1, 7 } };
  filter = Run(MakeImage(oneConstant), [](vtkMultiVariateToRGBA* f) { SetGroups(f, { 3, 4 }); });
  CHECK(Finite(filter));
  Rows normalized;
  for (const auto& row : oneConstant)
  {
    normalized.push_back({ row[0] / 4, row[1] / 4, row[2] / 4, 0 });
  }
  CHECK(AlphaIs(filter, MeanAlpha(normalized)));

  // Each component in a group of its own is normalized to its own range
  filter =
    Run(MakeImage(oneConstant), [](vtkMultiVariateToRGBA* f) { SetGroups(f, { 1, 2, 3, 4 }); });
  CHECK(Finite(filter));
  // the ranges of the first three columns: [0, 4], [0, 3], [0, 4]
  const double spread[] = { 4, 3, 4 };
  normalized.clear();
  for (const auto& row : oneConstant)
  {
    normalized.push_back({ row[0] / spread[0], row[1] / spread[1], row[2] / spread[2], 0 });
  }
  CHECK(AlphaIs(filter, MeanAlpha(normalized)));

  // Integer arrays that reach the limits of their type give the same result
  // as the same values in a double array
  const Rows uint8Rows = { { 0, 255, 128, 1 }, { 255, 0, 1, 128 }, { 10, 20, 30, 40 },
    { 255, 255, 0, 0 } };
  const Rows int8Rows = { { -128, 127, 0, 1 }, { 127, -128, 1, 0 }, { 10, 20, 30, 40 },
    { -128, -128, 0, 0 } };
  for (const auto& [rows, dataType] :
    { std::make_pair(uint8Rows, VTK_UNSIGNED_CHAR), std::make_pair(int8Rows, VTK_SIGNED_CHAR) })
  {
    auto integer = Run(MakeImage(rows, dataType));
    auto reference = Run(MakeImage(rows));
    CHECK(Finite(integer));
    CHECK(Equal(GetArray(integer, "rgba"), GetArray(reference, "rgba")));
    CHECK(Equal(GetArray(integer, "gbc"), GetArray(reference, "gbc")));
  }

  // A single point: in one group the components are spread over [0, 1]; in
  // separate groups each is constant, so the point is at the center.
  filter = Run(MakeImage({ { 1, 2, 3, 4 } }));
  CHECK(Finite(filter));
  CHECK(AlphaIs(filter, { 0.5 }));
  filter = Run(
    MakeImage({ { 1, 2, 3, 4 } }), [](vtkMultiVariateToRGBA* f) { SetGroups(f, { 1, 2, 3, 4 }); });
  CHECK(Finite(filter));
  CHECK(AtCenter(filter));
  CHECK(AlphaIs(filter, { 0 }));

  // No points: the output arrays are empty
  vtkNew<vtkPolyData> empty;
  vtkNew<vtkDoubleArray> emptyValues;
  emptyValues->SetName("mv");
  emptyValues->SetNumberOfComponents(4);
  empty->GetPointData()->AddArray(emptyValues);
  filter = Run(empty);
  CHECK(GetArray(filter, "rgba") && GetArray(filter, "rgba")->GetNumberOfTuples() == 0);
  CHECK(GetArray(filter, "gbc") && GetArray(filter, "gbc")->GetNumberOfTuples() == 0);

  // Fewer than 3 components, in the array or visible: the filter fails
  // without adding arrays
  filter = Run(MakeImage({ { 0, 1 }, { 1, 0 }, { 2, 2 } }));
  CHECK(!GetArray(filter, "rgba") && !GetArray(filter, "gbc"));
  filter = Run(MakeImage(oneConstant),
    [](vtkMultiVariateToRGBA* f)
    {
      f->AddVisibleVariableIndex(0);
      f->AddVisibleVariableIndex(1);
    });
  CHECK(!GetArray(filter, "rgba") && !GetArray(filter, "gbc"));

  // Invalid partitions of the 4 components are rejected
  auto image = MakeImage(oneConstant);
  const std::string invalidPartition = "Invalid partition";
  // duplicate, missing, out of range and negative indices
  CHECK(Rejected(image, Partition({ 0, 0, 1, 2 }, { 4 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2 }, { 3 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2, 4 }, { 4 }), invalidPartition));
  CHECK(Rejected(image, Partition({ -1, 0, 1, 2 }, { 4 }), invalidPartition));
  // missing, empty, decreasing, short and long offsets, and offsets without indices
  CHECK(Rejected(image, Partition({ 0, 1, 2, 3 }, {}), invalidPartition));
  CHECK(Rejected(image, Partition({}, { 4 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2, 3 }, { 0, 4 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2, 3 }, { 2, 2, 4 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2, 3 }, { 3, 1, 4 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2, 3 }, { 2, 3 }), invalidPartition));
  CHECK(Rejected(image, Partition({ 0, 1, 2, 3 }, { 2, 5 }), invalidPartition));

  // A CustomRange without one [min, max] pair per group is rejected
  const std::string invalidRange = "Invalid CustomRange";
  CHECK(Rejected(image, [](vtkMultiVariateToRGBA* f) { f->SetCustomRange({ 0 }); }, invalidRange));
  CHECK(Rejected(
    image, [](vtkMultiVariateToRGBA* f) { f->SetCustomRange({ 0, 1, 0, 1 }); }, invalidRange));
  CHECK(Rejected(
    image,
    [](vtkMultiVariateToRGBA* f)
    {
      SetGroups(f, { 3, 4 });
      f->AddCustomRange(0, 4);
    },
    invalidRange));
  // one pair per group is accepted
  filter = Run(image,
    [](vtkMultiVariateToRGBA* f)
    {
      SetGroups(f, { 3, 4 });
      f->AddCustomRange(0, 4);
      f->AddCustomRange(0, 8);
    });
  CHECK(Finite(filter));
  // a pair with min > max is rejected, in any group
  CHECK(
    Rejected(image, [](vtkMultiVariateToRGBA* f) { f->SetCustomRange({ 4, 0 }); }, invalidRange));
  CHECK(Rejected(
    image,
    [](vtkMultiVariateToRGBA* f)
    {
      SetGroups(f, { 3, 4 });
      f->AddCustomRange(0, 4);
      f->AddCustomRange(8, 0);
    },
    invalidRange));
  // min == max, as GetRange reports for a constant group, maps every value to 0
  filter = Run(image, [](vtkMultiVariateToRGBA* f) { f->SetCustomRange({ 2, 2 }); });
  CHECK(Finite(filter));
  CHECK(AtCenter(filter));
  CHECK(AlphaIs(filter, { 0, 0, 0, 0 }));

  // Invalid output selections are rejected
  CHECK(Rejected(
    image, [](vtkMultiVariateToRGBA* f) { f->SetStoreA(true); }, "both StoreRGBA and StoreA"));
  CHECK(Rejected(
    image,
    [](vtkMultiVariateToRGBA* f)
    {
      f->SetStoreGBC(false);
      f->SetStoreRGBA(false);
    },
    "all set to false"));

  // A rejected filter recovers once its configuration is fixed
  filter = Run(image, Partition({ 0, 1, 2 }, { 3 }));
  CHECK(!GetArray(filter, "rgba"));
  filter->SetPartitionIndices({ 0, 1, 2, 3 });
  filter->SetPartitionOffsets({ 3, 4 });
  filter->Update();
  CHECK(Finite(filter));

  // No input: the filter fails without crashing
  vtkNew<vtkMultiVariateToRGBA> noInput;
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  noInput->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  noInput->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  noInput->SetController(nullptr);
  noInput->Update();
  CHECK(errorObserver->GetError());
  return EXIT_SUCCESS;
}
