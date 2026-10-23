// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtkmDataArray::NewVTKArray() turns each kind of Viskores handle into the
// VTK array that holds the same values, and the copy outlives the original.
// Data that are only on a device are brought to the host once, and the host
// copy is kept in the handle.

#include "vtkAOSDataArrayTemplate.h"
#include "vtkAffineArray.h"
#include "vtkConstantArray.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"

#include "vtkmDataArray.h"
#include "vtkmDataArrayUtilities.h"

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleSOA.h>

#include <iostream>
#include <vector>

namespace
{
template <typename T>
vtkSmartPointer<vtkmDataArray<T>> Wrap(const viskores::cont::UnknownArrayHandle& handle)
{
  auto array = vtkSmartPointer<vtkmDataArray<T>>::New();
  array->SetVtkmArrayHandle(handle);
  return array;
}

std::vector<double> Values(vtkDataArray* array)
{
  std::vector<double> values;
  for (vtkIdType t = 0; t < array->GetNumberOfTuples(); ++t)
  {
    for (int c = 0; c < array->GetNumberOfComponents(); ++c)
    {
      values.push_back(array->GetComponent(t, c));
    }
  }
  return values;
}

// Convert @a original, release it, and check that the copy is an @a Expected
// with the values @a original had. With @a readFirst, @a original is read
// through the vtkDataArray API before it is converted, which is what any VTK
// code that looks at it does.
template <typename Expected, typename T>
bool Check(vtkSmartPointer<vtkmDataArray<T>> original, const char* what, bool readFirst = false)
{
  std::vector<double> expected;
  if (readFirst)
  {
    expected = Values(original);
  }
  vtkSmartPointer<vtkDataArray> copy;
  copy.TakeReference(original->NewVTKArray());
  if (!readFirst)
  {
    expected = Values(original);
  }
  const int numComponents = original->GetNumberOfComponents();
  original = nullptr;

  if (!copy)
  {
    std::cerr << what << ": no copy\n";
    return false;
  }
  if (!Expected::SafeDownCast(copy))
  {
    std::cerr << what << ": unexpected " << copy->GetClassName() << "\n";
    return false;
  }
  if (copy->GetNumberOfComponents() != numComponents || Values(copy) != expected)
  {
    std::cerr << what << ": values differ\n";
    return false;
  }
  return true;
}
}

int TestVTKMNewVTKArray(int, char*[])
{
  using Vec3 = viskores::Vec3f_64;
  const std::vector<double> x{ 0, 1, 2, 3 }, y{ 4, 5, 6, 7 }, z{ 8, 9, 10, 11 };
  const std::vector<Vec3> xyz{ { 0, 4, 8 }, { 1, 5, 9 }, { 2, 6, 10 }, { 3, 7, 11 } };
  const std::vector<viskores::Id> reversed{ 3, 2, 1, 0 };

  bool ok = true;
  ok &= Check<vtkConstantArray<double>>(
    Wrap<double>(viskores::cont::make_ArrayHandleConstant(4.5, 10)), "constant");
  ok &= Check<vtkAffineArray<double>>(
    Wrap<double>(viskores::cont::make_ArrayHandleCounting(2.0, 0.5, 10)), "counting");
  ok &= Check<vtkAffineArray<viskores::Id>>(
    Wrap<viskores::Id>(viskores::cont::ArrayHandleIndex(10)), "index");
  auto soa = [&]()
  {
    return Wrap<double>(viskores::cont::make_ArrayHandleSOA<Vec3>(
      { viskores::cont::make_ArrayHandle(x, viskores::CopyFlag::On),
        viskores::cont::make_ArrayHandle(y, viskores::CopyFlag::On),
        viskores::cont::make_ArrayHandle(z, viskores::CopyFlag::On) }));
  };
  ok &= Check<vtkSOADataArrayTemplate<double>>(soa(), "struct of arrays");
  ok &= Check<vtkSOADataArrayTemplate<double>>(soa(), "struct of arrays, read first", true);
  ok &= Check<vtkAOSDataArrayTemplate<double>>(
    Wrap<double>(viskores::cont::make_ArrayHandle(xyz, viskores::CopyFlag::On)), "interleaved");
  ok &= Check<vtkAOSDataArrayTemplate<double>>(
    Wrap<double>(viskores::cont::make_ArrayHandlePermutation(
      viskores::cont::make_ArrayHandle(reversed, viskores::CopyFlag::On),
      viskores::cont::make_ArrayHandle(x, viskores::CopyFlag::On))),
    "permutation");

  // Data written on a device, and so only there: asking for the device
  // pointer gets the write pointer, which leaves the host copy stale.
  auto handle = viskores::cont::make_ArrayHandle(x, viskores::CopyFlag::On);
  auto onDevice = Wrap<double>(handle);
  onDevice->GetDeviceVoidPointer(0);
  const viskores::cont::internal::Buffer& buffer = handle.GetBuffers()[0];
  const bool gpu = vtkmDataArrayUtilities::GetDeviceMemorySpaceName() &&
    (vtkmDataArrayUtilities::IsDeviceAdapterAvailable(VISKORES_DEVICE_ADAPTER_CUDA) ||
      vtkmDataArrayUtilities::IsDeviceAdapterAvailable(VISKORES_DEVICE_ADAPTER_KOKKOS));
  std::cout << "Device-only data " << (gpu ? "on the GPU" : "skipped: no GPU") << "\n";
  if (gpu)
  {
    if (buffer.IsAllocatedOnHost())
    {
      std::cerr << "device: data are still on the host\n";
      ok = false;
    }
    vtkSmartPointer<vtkDataArray> copy;
    copy.TakeReference(onDevice->NewVTKArray());
    if (!vtkAOSDataArrayTemplate<double>::SafeDownCast(copy) || Values(copy) != x)
    {
      std::cerr << "device: wrong copy\n";
      ok = false;
    }
    if (!buffer.IsAllocatedOnHost())
    {
      std::cerr << "device: the host copy was not kept\n";
      ok = false;
    }
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
