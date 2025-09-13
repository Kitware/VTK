// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataAlgorithm.h"

#include <array>
#include <iostream>

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

namespace
{

// This is an example of how filters can use `vtkAlgorithm::GetInputArray`
// to avoid having to extract a component or norm from a multi-component
// array.
//
// It also shows how filters may validate the number of arrays they are
// configured to process (or allow users to provide an arbitrary number
// of arrays to process).
class vtkDummyFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkDummyFilter, vtkPolyDataAlgorithm);
  static vtkDummyFilter* New();
  void PrintSelf(std::ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

protected:
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    (void)request;

    auto* input = vtkPolyData::GetData(inputVector[0]);
    auto* output = vtkPolyData::GetData(outputVector, 0);

    int numArrays = this->GetNumberOfInputArraySpecifications();
    std::cout << "RequestData configured to process " << numArrays << " arrays.\n";
    if (numArrays != 3)
    {
      vtkErrorMacro("Expected 3 array specifications, got " << numArrays << ".");
      return 0;
    }

    int association = -1;
    auto a1 = this->GetInputArrayAs<vtkDataArray>(0, input, association);
    auto a2 = this->GetInputArrayAs<vtkDataArray>(1, input, association, 0);
    // On the first run, a component is specified with the array, so LInfNorm
    // is ignored. But on the second run, we force the LInfNorm of each tuple
    // to be used as a virtual component:
    auto a3 =
      this->GetInputArrayAs<vtkDataArray>(2, input, association, vtkArrayComponents::LInfNorm);
    if (!a1 || !a2 || !a3)
    {
      vtkErrorMacro("Expected 3 arrays, got " << a1 << " " << a2 << " " << a3 << ".");
      return 0;
    }
    vtkIdType nn = a1->GetNumberOfTuples();
    std::cout << "Array sizes:\n"
              << "  a1 " << a1->GetNumberOfTuples() << " " << a1->GetNumberOfComponents() << "  a2 "
              << a2->GetNumberOfTuples() << " " << a2->GetNumberOfComponents() << "  a3 "
              << a3->GetNumberOfTuples() << " " << a3->GetNumberOfComponents() << "\n";
    if (a1->GetNumberOfComponents() != 1 || a2->GetNumberOfComponents() != 1 ||
      a3->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Expected arrays to have a single component.");
    }
    if (a2->GetNumberOfTuples() != nn || a3->GetNumberOfTuples() != nn)
    {
      vtkErrorMacro("Expected all arrays to have " << nn << " tuples.");
    }
    output->ShallowCopy(input);
    vtkNew<vtkDoubleArray> result;
    result->SetName("foo");
    result->SetNumberOfTuples(nn);
    for (vtkIdType ii = 0; ii < nn; ++ii)
    {
      result->SetValue(ii, a1->GetTuple1(ii) * a2->GetTuple1(ii) / a3->GetTuple1(ii));
    }
    output->GetPointData()->SetScalars(result);
    return 1;
  }
};

} // anonymous namespace

vtkStandardNewMacro(vtkDummyFilter);

// Test vtkAlgorithm's array-processing APIs that accept components:
// + query the number of input arrays inside a filter (`GetNumberOfInputArraySpecifications()`),
// + `GetInputArray()` and `GetInputArrayAs<>()` for some trivial cases,
// + that when an input data-array is a single component, no implicit array is created.
// + resetting the input array specifications works.
// + calling `GetInputArray()` with a requested component works.
int TestMultipleInputArrayComponents(int, char*[])
{
  // I. Prepare input data with 4 points and 3 point-data scalar-arrays.
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  // clang-format off
  std::array<std::array<double, 3>, 4> coords{ {
    { 0, 0, 0 },
    { 1, 0, 0 },
    { 1, 1, 0 },
    { 0, 1, 0 } } };
  // clang-format on
  vtkNew<vtkDoubleArray> a1;
  vtkNew<vtkDoubleArray> a2;
  vtkNew<vtkDoubleArray> a3;
  a1->SetName("a1");
  a2->SetName("a2");
  a3->SetName("a3");
  a1->SetNumberOfTuples(points->GetNumberOfPoints());
  a2->SetNumberOfTuples(points->GetNumberOfPoints());
  a3->SetNumberOfComponents(3);
  a3->SetNumberOfTuples(points->GetNumberOfPoints());
  // clang-format off
  std::array<std::array<double, 3>, 4> ptdata{ {
    {-5, -3, 1 },
    { 7,  4, 2 },
    { 9,  1, 3 },
    { 8,  8, 4 } } };
  // clang-format on
  for (vtkIdType ii = 0; ii < 4; ++ii)
  {
    points->SetPoint(ii, coords[ii].data());
    a1->SetValue(ii, ptdata[ii][0]);
    a2->SetValue(ii, ptdata[ii][1]);
    a3->SetTuple(ii, ptdata[ii].data());
  }

  polyData->SetPoints(points);
  polyData->GetPointData()->AddArray(a1);
  polyData->GetPointData()->AddArray(a2);
  polyData->GetPointData()->AddArray(a3);

  // II. Create our test filter and configure it to process 3 arrays:
  vtkNew<vtkDummyFilter> filter;
  filter->SetInputDataObject(0, polyData);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a1");
  filter->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a2", 0);
  filter->SetInputArrayToProcess(2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a3", 2);
  // See vtkDummyFilter::RequestData for testing that is performed during Update():
  filter->Update();

  // III. Fetch the filter output and validate it.
  auto* output = filter->GetOutput();
  if (!output)
  {
    std::cerr << "No output data.\n";
    return TEST_FAILURE;
  }
  auto* opd = output->GetPointData();
  auto* foo = vtkDoubleArray::SafeDownCast(opd->GetArray("foo"));
  if (!foo || foo->GetNumberOfTuples() != 4)
  {
    std::cerr << "No output array.\n";
    return TEST_FAILURE;
  }
  std::array<double, 4> expected{ 15, 14, 3, 16 };
  bool ok = true;
  for (vtkIdType ii = 0; ii < 4; ++ii)
  {
    std::cout << "  " << ii << " " << foo->GetValue(ii) << "\n";
    if (foo->GetValue(ii) != expected[ii])
    {
      std::cerr << "    ERROR:  Expected " << expected[ii] << ".\n";
      ok = false;
    }
  }

  // IV. Reset and run again with different input arrays
  filter->ResetInputArraySpecifications();
  if (filter->GetNumberOfInputArraySpecifications() != 0)
  {
    std::cerr << "ERROR: Failed to reset input array specifications.\n";
    return TEST_FAILURE;
  }
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a2");
  filter->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a1", 0);
  // This time, we don't specify a component, triggering the "requestedComponent" parameter
  // of vtkAlgorithm::GetInputArray() to activate in RequestData():
  filter->SetInputArrayToProcess(2, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a3");
  filter->Update();
  output = filter->GetOutput();
  opd = output->GetPointData();
  foo = vtkDoubleArray::SafeDownCast(opd->GetArray("foo"));
  if (!foo || foo->GetNumberOfTuples() != 4)
  {
    std::cerr << "No output array.\n";
    return TEST_FAILURE;
  }
  std::array<double, 4> expected2{ 3, 4, 1, 8 };
  ok = true;
  for (vtkIdType ii = 0; ii < 4; ++ii)
  {
    std::cout << "  " << ii << " " << foo->GetValue(ii) << "\n";
    if (foo->GetValue(ii) != expected2[ii])
    {
      std::cerr << "    ERROR:  Expected " << expected2[ii] << ".\n";
      ok = false;
    }
  }

  return ok ? TEST_SUCCESS : TEST_FAILURE;
}
