// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that a lens-only change of vtkMultiVariateToRGBA reuses the last
 * results and gives the same output as a full execution.
 */

#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkTrivialProducer.h>
#include <vtkVariant.h>

#include <cmath>
#include <iostream>

namespace
{
#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Line " << __LINE__ << ": check failed: " #condition << std::endl;                \
    return EXIT_FAILURE;                                                                           \
  }

vtkDataArray* GetArray(vtkMultiVariateToRGBA* filter, const char* name)
{
  return vtkImageData::SafeDownCast(filter->GetOutputDataObject(0))->GetPointData()->GetArray(name);
}

bool Equal(vtkDataArray* a, vtkDataArray* b, double tolerance = 0)
{
  if (a->GetNumberOfValues() != b->GetNumberOfValues())
  {
    return false;
  }
  for (vtkIdType i = 0; i < a->GetNumberOfValues(); ++i)
  {
    if (std::fabs(a->GetVariantValue(i).ToDouble() - b->GetVariantValue(i).ToDouble()) > tolerance)
    {
      return false;
    }
  }
  return true;
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

  // 'cached' only executes fully once, 'full' every time
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
  vtkDataArray* gbc = GetArray(cached, "gbc");

  SetLens(cached, full, 0.4, -0.2, 0.5);
  CHECK(GetArray(cached, "gbc") == gbc);
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

  // float output, for full and lens-only executions
  cached->SetOutputScalarType(VTK_FLOAT);
  SetLens(cached, full, 0.3, 0.3, 0.6);
  CHECK(GetArray(cached, "rgba")->GetDataType() == VTK_FLOAT);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba"), 1e-6));
  SetLens(cached, full, 0.2, 0.1, 0.5);
  CHECK(GetArray(cached, "rgba")->GetDataType() == VTK_FLOAT);
  CHECK(Equal(GetArray(cached, "rgba"), GetArray(full, "rgba"), 1e-6));

  // StoreA: RGB is 0 and alpha is the same as with StoreRGBA
  cached->SetStoreRGBA(false);
  cached->SetStoreA(true);
  cached->SetOutputScalarType(VTK_DOUBLE);
  SetLens(cached, full, 0.2, 0.2, 0.5);
  vtkDataArray* a = GetArray(cached, "rgba");
  vtkDataArray* rgba = GetArray(full, "rgba");
  for (vtkIdType i = 0; i < a->GetNumberOfTuples(); ++i)
  {
    CHECK(a->GetComponent(i, 0) == 0 && a->GetComponent(i, 1) == 0 && a->GetComponent(i, 2) == 0);
    CHECK(a->GetComponent(i, 3) == rgba->GetComponent(i, 3));
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
  return EXIT_SUCCESS;
}
