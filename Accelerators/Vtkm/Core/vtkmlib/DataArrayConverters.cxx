// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "DataArrayConverters.hxx"

#include "vtkmDataArray.h"

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>

#include "vtkDataArray.h"
#include "vtkPoints.h"

#include <cstdint>
#include <limits>

// If the Viskores devices share memory with the host, then we can provide VTK with efficient
// memory structures without unnecessary copies.
// TODO: Provide a better way for Viskores to declare whether device arrays are unified.
// TODO: Can we use unified memory with Kokkos?
#if !defined(VISKORES_KOKKOS_CUDA) && !defined(VISKORES_KOKKOS_HIP)
#define VISKORES_UNIFIED_MEMORY 1
#endif

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

namespace
{

struct ArrayConverter
{
  template <typename T>
  void operator()(T, const viskores::cont::UnknownArrayHandle& input, vtkDataArray*& output) const
  {
    if ((output == nullptr) && input.IsBaseComponentType<T>())
    {
      if (input.CanConvert<viskores::cont::ArrayHandleRuntimeVec<T>>())
      {
        output = this->MakeAOSArray<T>(input);
      }
      else if (input.IsStorageType<viskores::cont::StorageTagSOA>())
      {
        output = this->MakeSOAArray<T>(input);
      }
      else
      {
        output = this->MakeVtkmData<T>(input);
      }
    }
  }

private:
  template <typename T>
  vtkmDataArray<T>* MakeVtkmData(const viskores::cont::UnknownArrayHandle& input) const
  {
    vtkmDataArray<T>* output = vtkmDataArray<T>::New();
    output->SetVtkmArrayHandle(input);
    return output;
  }

  template <typename T>
  vtkDataArray* MakeAOSArray(viskores::cont::UnknownArrayHandle input) const
  {
    // We can steal this array (probably)!
    using VTKArrayType = vtkAOSDataArrayTemplate<T>;

    viskores::cont::ArrayHandleRuntimeVec<T> runtimeVecArray{ input.GetNumberOfComponentsFlat() };
    input.AsArrayHandle(runtimeVecArray);

    viskores::cont::ArrayHandleBasic<T> componentsArray = runtimeVecArray.GetComponentsArray();
    viskores::Id size = componentsArray.GetNumberOfValues();

    vtkNew<VTKArrayType> output;
    output->SetNumberOfComponents(runtimeVecArray.GetNumberOfComponents());

    // Basic arrays have a single buffer containing the unadulterated data.
    viskores::cont::internal::Buffer buffer = componentsArray.GetBuffers()[0];

    // If the Viskores device supports unified memory, then it is OK if the data are on the
    // device. Getting the host pointer will just get the same pointer on the device, and the
    // data will be paged in as requested (if ever requested). However, if the Viskores device
    // does not support unified memory, then this will require a perhaps unnecessary memory
    // copy. Instead, wrap the Viskores array in a vtkmDataArray. This may slow down VTK access
    // if that is later needed. Note that it is possible for the data to be on both host
    // and device. In this case, the device data may get removed, but that seems like
    // a reasonable compromise.
#ifndef VISKORES_UNIFIED_MEMORY
    if (!buffer.IsAllocatedOnHost())
    {
      return this->MakeVtkmData<T>(input);
    }
#endif //! VISKORES_UNIFIED_MEMORY

    viskores::cont::internal::TransferredBuffer transfer = buffer.TakeHostBufferOwnership();
    auto srcMemory = reinterpret_cast<T*>(transfer.Memory);
    assert(transfer.Size >= (size * sizeof(T)));
    if (transfer.Memory == transfer.Container)
    { // transfer the memory ownership over to VTK instead of copy
      output->SetVoidArray(srcMemory, size, 0, vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
      output->SetArrayFreeFunction(transfer.Delete);
    }
    else
    {
      // deep copy the memory to VTK as the memory coming from
      // a source that VTK can't represent
      output->SetNumberOfValues(componentsArray.GetNumberOfValues());
      std::copy(srcMemory, srcMemory + size, output->GetPointer(0));
      transfer.Delete(transfer.Container);
    }

    output->Register(nullptr);
    return output.GetPointer();
  }

  template <typename T>
  vtkDataArray* MakeSOAArray(viskores::cont::UnknownArrayHandle input) const
  {
    // We can steal this array (probably)!
    using VTKArrayType = vtkSOADataArrayTemplate<T>;

    viskores::IdComponent numComponents = input.GetNumberOfComponentsFlat();
    viskores::Id size = input.GetNumberOfValues();

    if (static_cast<std::size_t>(size) >=
      static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(T))
    {
      std::ostringstream err;
      err << "Allocation request too big: " << size << " elements of " << sizeof(T) << " bytes";
      throw viskores::cont::ErrorBadAllocation(err.str());
    }

    vtkNew<VTKArrayType> output;
    output->SetNumberOfComponents(numComponents);

    // We cannot get an `ArrayHandleSOA` directly because we do not know the number of
    // components at compile time. Instead, extract each component as an `ArrayHandleStride`.
    // If the `UnknownArrayHandle` contains an `ArrayHandleSOA`, each component array
    // should have a stride of 1.
    for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      viskores::cont::ArrayHandleStride<T> strideArray = input.ExtractComponent<T>(cIndex);
      if ((strideArray.GetStride() != 1) || (strideArray.GetOffset() != 0) ||
        (strideArray.GetModulo() != 0) || (strideArray.GetDivisor() != 1))
      {
        // Unexpected layout of the stride array. Perhaps this is an SOA of a nested Vec
        // and only the outer Vec is strided. In this case, give up.
        return this->MakeVtkmData<T>(input);
      }

      viskores::cont::ArrayHandleBasic<T> componentArray = strideArray.GetBasicArray();

      // Basic arrays have a single buffer containing the unadulterated data.
      viskores::cont::internal::Buffer buffer = componentArray.GetBuffers()[0];

      // If the Viskores device supports unified memory, then it is OK if the data are on the
      // device. Getting the host pointer will just get the same pointer on the device, and the
      // data will be paged in as requested (if ever requested). However, if the Viskores device
      // does not support unified memory, then this will require a perhaps unnecessary memory
      // copy. Instead, wrap the Viskores array in a vtkmDataArray. This may slow down VTK access
      // if that is later needed. Note that it is possible for the data to be on both host
      // and device. In this case, the device data may get removed, but that seems like
      // a reasonable compromise.
#ifndef VISKORES_UNIFIED_MEMORY
      if ((cIndex == 0) && !buffer.IsAllocatedOnHost())
      {
        return this->MakeVtkmData<T>(input);
      }
#endif //! VISKORES_UNIFIED_MEMORY

      viskores::cont::internal::TransferredBuffer transfer = buffer.TakeHostBufferOwnership();
      auto srcMemory = reinterpret_cast<T*>(transfer.Memory);
      assert(transfer.Size >= (size * sizeof(T)));
      if (transfer.Memory == transfer.Container)
      { // transfer the memory ownership over to VTK instead of copy
        output->SetArray(
          cIndex, srcMemory, size, true, 0, vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
        output->SetArrayFreeFunction(cIndex, transfer.Delete);
      }
      else
      {
        // deep copy the memory to VTK as the memory coming from
        // a source that VTK can't represent
        T* dataBuffer = new T[size];
        std::copy(srcMemory, srcMemory + size, dataBuffer);

        output->SetArray(
          cIndex, dataBuffer, size, true, 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
        transfer.Delete(transfer.Container);
      }
    }

    output->Register(nullptr);
    return output.GetPointer();
  }
};
} // anonymous namespace

// Though the following conversion routines take const-ref parameters as input,
// the underlying storage will be stolen, whenever possible, instead of
// performing a full copy.
// Therefore, these routines should be treated as "moves" and the state of the
// input is undeterminisitic.

vtkDataArray* Convert(const viskores::cont::Field& input)
{
  return Convert(input.GetData(), input.GetName());
}

vtkDataArray* Convert(const viskores::cont::UnknownArrayHandle& input, const std::string& name)
{
  // We need to do the conversion from UnknownArrayHandle to a known viskores::cont::ArrayHandle
  // after that we need to fill the vtkDataArray
  vtkDataArray* output = nullptr;

  try
  {
    viskores::ListForEach(ArrayConverter{}, toviskores::VTKScalarTypes{}, input, output);
    if (output)
    {
      if (!name.empty() && (name != tovtkm::NoNameVTKFieldName()))
      {
        output->SetName(name.c_str());
      }
    }
    else
    {
      vtkGenericWarningMacro("Could not determine value type for array " << name);
      input.PrintSummary(std::cout);
    }
  }
  catch (viskores::cont::Error& e)
  {
    vtkGenericWarningMacro(
      "Encountered error while converting Viskores array " << name << ": " << e.what());
  }
  return output;
}

vtkPoints* Convert(const viskores::cont::CoordinateSystem& input)
{
  vtkDataArray* data = Convert(input.GetData(), input.GetName());
  if (data)
  {
    vtkPoints* points = vtkPoints::New();
    points->SetData(data);
    data->FastDelete();
    return points;
  }
  else
  {
    vtkGenericWarningMacro("Converting viskores::cont::CoordinateSystem to vtkPoints failed");
    return nullptr;
  }
}

VTK_ABI_NAMESPACE_END
}
