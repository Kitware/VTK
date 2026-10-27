// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that a lens-only change of vtkMultiVariateToRGBA reuses the last
 * results and output object, and gives the same output as a full execution,
 * and that the output object follows the input type.
 */

#include <vtkDataArrayRange.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTrivialProducer.h>

#include <algorithm>
#include <cmath>
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

// Returns the point data array 'name' of the output of 'filter'.
vtkDataArray* GetArray(vtkMultiVariateToRGBA* filter, const char* name)
{
  return vtkImageData::SafeDownCast(filter->GetOutputDataObject(0))->GetPointData()->GetArray(name);
}

// Returns true if 'a' and 'b' have the same number of values and the
// values differ by at most 'tolerance'.
bool Equal(vtkDataArray* a, vtkDataArray* b, double tolerance = 0)
{
  auto aValues = vtk::DataArrayValueRange(a);
  auto bValues = vtk::DataArrayValueRange(b);
  return aValues.size() == bValues.size() &&
    std::equal(aValues.begin(), aValues.end(), bValues.begin(),
      [tolerance](double x, double y) { return std::fabs(x - y) <= tolerance; });
}

// Sets the same lens on both filters and updates them, 'full' with a full
// execution.
void SetLens(
  vtkMultiVariateToRGBA* cached, vtkMultiVariateToRGBA* full, double x, double y, double r)
{
  for (vtkMultiVariateToRGBA* filter : { cached, full })
  {
    filter->SetLensCenter(x, y);
    filter->SetLensRadius(r);
    filter->SetOutsideLensAlpha(0.1);
  }
  // not lens-only: 'full' cannot reuse its results, unlike 'cached'
  full->Modified();
  cached->Update();
  full->Update();
}
}

int TestMultiVariateToRGBALens(int, char*[])
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(10, 10, 10);
  vtkNew<vtkDoubleArray> values;
  values->SetNumberOfComponents(4);
  values->SetNumberOfTuples(image->GetNumberOfPoints());
  for (vtkIdType i = 0; i < values->GetNumberOfValues(); ++i)
  {
    values->SetValue(i, std::fabs(std::sin(0.37 * i)));
  }
  image->GetPointData()->SetScalars(values);
  vtkNew<vtkImageData> other;
  other->DeepCopy(image);
  other->GetPointData()->GetScalars()->SetComponent(0, 0, 0.9);

  // 'cached' reuses its results for lens-only changes, 'full' always executes fully
  vtkNew<vtkMultiVariateToRGBA> cached;
  vtkNew<vtkMultiVariateToRGBA> full;
  for (vtkMultiVariateToRGBA* filter : { cached.GetPointer(), full.GetPointer() })
  {
    filter->SetController(nullptr);
    filter->SetInputData(image);
    filter->SetStoreGBC(true);
  }
  vtkNew<vtkTrivialProducer> producer;
  producer->SetOutput(image);
  cached->SetInputConnection(producer->GetOutputPort());
  cached->Update();
  // holds a reference, as a full execution releases the cached array
  vtkSmartPointer<vtkDataArray> gbc = GetArray(cached, "gbc");
  vtkDataObject* output = cached->GetOutputDataObject(0);

  SetLens(cached, full, 0.4, -0.2, 0.5);
  // the same array: the results were reused from the cache
  CHECK(GetArray(cached, "gbc") == gbc);
  CHECK(cached->GetOutputDataObject(0) == output);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba")));

  // a change that is not lens-only executes fully
  cached->SetVerticesRotation(30);
  full->SetVerticesRotation(30);
  SetLens(cached, full, 0.1, 0.3, 0.6);
  CHECK(GetArray(cached, "gbc") != gbc);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba")));

  // so does a switch to another input, even one older than the last execution
  producer->SetOutput(other);
  full->SetInputData(other);
  SetLens(cached, full, 0.1, 0.2, 0.6);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba")));
  producer->SetOutput(image);
  full->SetInputData(image);

  // so does a change of the input values
  gbc = GetArray(cached, "gbc");
  values->SetValue(7, 0.5);
  values->Modified();
  SetLens(cached, full, 0.2, 0.3, 0.6);
  CHECK(GetArray(cached, "gbc") != gbc);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba")));

  // inverting the lens is a lens-only change too: the points inside get the cap
  gbc = GetArray(cached, "gbc");
  cached->InvertLensOn();
  full->InvertLensOn();
  SetLens(cached, full, 0.2, 0.3, 0.6);
  // reused from the cache
  CHECK(GetArray(cached, "gbc") == gbc);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba")));
  vtkDataArray* inverted = GetArray(cached, "rgba");
  for (vtkIdType i = 0; i < gbc->GetNumberOfTuples(); ++i)
  {
    double* xy = gbc->GetTuple2(i);
    CHECK(std::hypot(xy[0] - 0.2, xy[1] - 0.3) > 0.6 || inverted->GetComponent(i, 3) <= 0.1);
  }
  cached->InvertLensOff();
  full->InvertLensOff();

  // float output, for full and lens-only executions
  cached->SetOutputScalarType(VTK_FLOAT);
  SetLens(cached, full, 0.3, 0.3, 0.6);
  CHECK(GetArray(cached, "rgba")->GetDataType() == VTK_FLOAT);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba"), 1e-6));
  SetLens(cached, full, 0.2, 0.1, 0.5);
  CHECK(GetArray(cached, "rgba")->GetDataType() == VTK_FLOAT);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba"), 1e-6));

  // StoreA: RGB is 0 and alpha is the same as with StoreRGBA, after a full
  // execution and after a lens-only change
  cached->SetStoreRGBA(false);
  cached->SetStoreA(true);
  cached->SetOutputScalarType(VTK_DOUBLE);
  for (bool lensOnly : { false, true })
  {
    gbc = GetArray(cached, "gbc");
    SetLens(cached, full, lensOnly ? -0.1 : 0.2, 0.2, 0.5);
    // reused from the cache
    CHECK(!lensOnly || GetArray(cached, "gbc") == gbc);
    vtkDataArray* a = GetArray(cached, "rgba");
    vtkDataArray* rgba = GetArray(full, "rgba");
    for (vtkIdType i = 0; i < a->GetNumberOfTuples(); ++i)
    {
      CHECK(a->GetComponent(i, 0) == 0 && a->GetComponent(i, 1) == 0 && a->GetComponent(i, 2) == 0);
      CHECK(a->GetComponent(i, 3) == rgba->GetComponent(i, 3));
    }
  }

  // values outside a custom range are clamped: alpha stays in [0, 1]
  // and the GBC inside the unit circle
  full->SetCustomRange({ 0.25, 0.5 });
  SetLens(cached, full, 0, 0, 1);
  double* alphaRange = GetArray(full, "rgba")->GetRange(3);
  CHECK(alphaRange[0] >= 0 && alphaRange[1] <= 1);
  vtkDataArray* gbcFull = GetArray(full, "gbc");
  for (vtkIdType i = 0; i < gbcFull->GetNumberOfTuples(); ++i)
  {
    double* xy = gbcFull->GetTuple2(i);
    CHECK(std::sqrt(xy[0] * xy[0] + xy[1] * xy[1]) <= 1 + 1e-9);
  }

  // the order of the lens setters does not matter
  cached->SetLensRadius(0.3);
  cached->SetLensCenter(1.5, 0.0);
  cached->SetLensRadius(0.6);
  full->SetLensRadius(0.3);
  full->SetLensRadius(0.6);
  full->SetLensCenter(1.5, 0.0);
  CHECK(cached->GetLensCenter()[0] == 1.5 && full->GetLensCenter()[0] == 1.5);

  // the output follows a switch to a table input and back
  vtkNew<vtkTable> table;
  for (int c = 0; c < 3; ++c)
  {
    vtkNew<vtkDoubleArray> column;
    column->SetName(std::to_string(c).c_str());
    column->SetNumberOfTuples(10);
    for (vtkIdType i = 0; i < 10; ++i)
    {
      column->SetValue(i, std::fabs(std::sin(0.37 * (i + 10 * c))));
    }
    table->AddColumn(column);
  }
  full->SetCustomRange({});
  full->SetInputData(table);
  full->Update();
  CHECK(vtkTable::SafeDownCast(full->GetOutputDataObject(0)));
  full->SetInputData(image);
  full->Update();
  CHECK(vtkImageData::SafeDownCast(full->GetOutputDataObject(0)));
  return EXIT_SUCCESS;
}
