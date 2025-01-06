// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitArrayUtilitiesDevice.h"
#include "vtkConduitArrayUtilitiesInternals.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDeviceMemoryType.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSetGet.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"

#include "vtkm/CellShape.h"
#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/ArrayHandleCast.h"
#include "vtkm/cont/CellSetSingleType.h"
#include "vtkmDataArray.h"
#include "vtkmlib/CellSetConverters.h"
#if defined(VTK_USE_CUDA)
#include <cuda_runtime_api.h>
#endif // VTK_USE_CUDA

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <type_traits>
#include <typeinfo>
#include <vector>

namespace internals
{
VTK_ABI_NAMESPACE_BEGIN

using vtkmConnectivityArrays = vtkTypeList::Unique<vtkTypeList::Create<vtkmDataArray<vtkm::Int8>,
  vtkmDataArray<vtkm::Int16>, vtkmDataArray<vtkm::Int32>, vtkmDataArray<vtkm::Int64>>>::Result;

void AddOneIndexToOffset(vtkm::cont::ArrayHandle<vtkm::Id>& offset, size_t connectivitySize)
{
  size_t lastValue = offset.GetNumberOfValues();
  offset.Allocate(lastValue + 1, vtkm::CopyFlag::On);
  auto portal = offset.WritePortal();
  portal.Set(lastValue, connectivitySize);
}

template <typename OutputValueT, typename ArrayT>
vtkm::cont::ArrayHandle<OutputValueT> ToArrayHandle(ArrayT* input)
{
  // input is a vtkmDataArray<inputValueType>
  auto inputUnknownHandle = input->GetVtkmUnknownArrayHandle();
  vtkm::cont::ArrayHandle<OutputValueT> output;
  vtkm::cont::ArrayCopyShallowIfPossible(inputUnknownHandle, output);
  return output;
}

//----------------------------------------------------------------------------
struct FromDeviceConduitToMonoShapedCellArray
{
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfPointsPerCell;
  int VTKCellType;
  vtkCellArray* CellArray;

  FromDeviceConduitToMonoShapedCellArray(vtkIdType numberOfPoints, vtkIdType numberOfPointsPerCell,
    int vtkCellType, vtkCellArray* cellArray)
    : NumberOfPoints(numberOfPoints)
    , NumberOfPointsPerCell(numberOfPointsPerCell)
    , VTKCellType(vtkCellType)
    , CellArray(cellArray)
  {
  }

  template <typename ArrayT>
  void operator()(ArrayT* input)
  {
    // input is a vtkmDataArray<inputValueType>
    vtkm::cont::CellSetSingleType<> cellSet;
    // VTK cell types and VTKm cell shapes have the same numbers
    cellSet.Fill(this->NumberOfPoints, this->VTKCellType, this->NumberOfPointsPerCell,
      internals::ToArrayHandle<vtkm::Id>(input));
    fromvtkm::Convert(cellSet, this->CellArray);
  }
};

//----------------------------------------------------------------------------
struct FromDeviceConduitToMixedCellArray
{
public:
  FromDeviceConduitToMixedCellArray(vtkIdType numberOfPoints, vtkCellArray* cellArray)
    : NumberOfPoints(numberOfPoints)
    , CellArray(cellArray)
  {
  }

  template <typename ArrayT1, typename ArrayT2, typename ArrayT3>
  void operator()(ArrayT1* offsets, ArrayT2* shapes, ArrayT3* connectivity)
  {
    // conduit offsets array does not include the last index =
    // connectivity.size() as CellSetExplicit
    vtkm::cont::CellSetExplicit<> cellSet;
    auto vtkmOffsets = ToArrayHandle<vtkm::Id>(offsets);
    auto vtkmConnectivity = ToArrayHandle<vtkm::Id>(connectivity);
    AddOneIndexToOffset(vtkmOffsets, vtkmConnectivity.GetNumberOfValues());
    cellSet.Fill(
      this->NumberOfPoints, ToArrayHandle<vtkm::UInt8>(shapes), vtkmConnectivity, vtkmOffsets);

    fromvtkm::Convert(cellSet, this->CellArray);
  }

private:
  vtkIdType NumberOfPoints;
  vtkCellArray* CellArray;
};

VTK_ABI_NAMESPACE_END
} // internals

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkConduitArrayUtilitiesDevice);
//----------------------------------------------------------------------------
vtkConduitArrayUtilitiesDevice::vtkConduitArrayUtilitiesDevice() = default;

//----------------------------------------------------------------------------
vtkConduitArrayUtilitiesDevice::~vtkConduitArrayUtilitiesDevice() = default;

#define vtkmAOSDataArrayConstructSingleComponent(dtype, nvals, raw_ptr, deviceAdapterId)           \
  do                                                                                               \
  {                                                                                                \
    return make_vtkmDataArray(vtkm::cont::ArrayHandle<dtype>(                                      \
      std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(                 \
        deviceAdapterId, reinterpret_cast<dtype*>(raw_ptr), reinterpret_cast<dtype*>(raw_ptr),     \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(nvals), [](void*) {},                 \
        vtkm::cont::internal::InvalidRealloc) }));                                                 \
  } while (0)

#define vtkmAOSDataArrayConstructMultiComponent(dtype, ntups, ncomp, raw_ptr, deviceAdapterId)     \
  do                                                                                               \
  {                                                                                                \
    return make_vtkmDataArray(vtkm::cont::ArrayHandle<vtkm::Vec<dtype, ncomp>>(                    \
      std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(                 \
        deviceAdapterId, reinterpret_cast<dtype*>(raw_ptr), reinterpret_cast<dtype*>(raw_ptr),     \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(ntups * ncomp), [](void*) {},         \
        vtkm::cont::internal::InvalidRealloc) }));                                                 \
  } while (0)

#define vtkmAOSDataArrayNumComponentsBody(dtype, ntups, ncomp, raw_ptr, deviceAdapterId)           \
  switch (num_components)                                                                          \
  {                                                                                                \
    case 1:                                                                                        \
      vtkmAOSDataArrayConstructSingleComponent(dtype, num_tuples, raw_ptr, deviceAdapterId);       \
    case 2:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 2, raw_ptr, deviceAdapterId);     \
    case 3:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 3, raw_ptr, deviceAdapterId);     \
    case 4:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 4, raw_ptr, deviceAdapterId);     \
    case 5:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 5, raw_ptr, deviceAdapterId);     \
    case 6:                                                                                        \
    default:                                                                                       \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 6, raw_ptr, deviceAdapterId);     \
  }

#define vtkmAOSDataArrayCase(conduitTypeId, dtype, ntups, ncomp, raw_ptr, deviceAdapterId)         \
  case conduitTypeId:                                                                              \
  {                                                                                                \
    vtkmAOSDataArrayNumComponentsBody(dtype, ntups, ncomp, raw_ptr, deviceAdapterId);              \
  }                                                                                                \
  break

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilitiesDevice::MCArrayToVTKmAOSArray(
  const conduit_node* c_mcarray, bool force_signed,
  const vtkm::cont::DeviceAdapterId& deviceAdapterId)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  const auto& child0 = mcarray.child(0);
  const conduit_cpp::DataType dtype0 = child0.dtype();

  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());
  void* raw_ptr = const_cast<void*>(child0.element_ptr(0));

  using conduit_dtype = conduit_cpp::DataType::Id;

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    vtkmAOSDataArrayCase(
      conduit_dtype::int8, vtkm::Int8, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::int16, vtkm::Int16, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::int32, vtkm::Int32, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::int64, vtkm::Int64, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::uint8, vtkm::UInt8, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::uint16, vtkm::UInt16, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::uint32, vtkm::UInt32, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::uint64, vtkm::UInt64, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::float32, vtkm::Float32, num_tuples, num_components, raw_ptr, deviceAdapterId);
    vtkmAOSDataArrayCase(
      conduit_dtype::float64, vtkm::Float64, num_tuples, num_components, raw_ptr, deviceAdapterId);
    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return {};
  }
  return {};
}

#define vtkmSOADataArrayConstructSingleComponent(dtype, nvals, deviceAdapterId)                    \
  do                                                                                               \
  {                                                                                                \
    std::vector<vtkm::cont::internal::Buffer> buffers;                                             \
    buffers.reserve(num_components);                                                               \
    for (int cc = 0; cc < num_components; ++cc)                                                    \
    {                                                                                              \
      buffers.push_back(vtkm::cont::internal::MakeBuffer(                                          \
        deviceAdapterId,                                                                           \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(nvals), [](void*) {},                 \
        vtkm::cont::internal::InvalidRealloc));                                                    \
    }                                                                                              \
    return make_vtkmDataArray(vtkm::cont::ArrayHandle<dtype>(buffers));                            \
  } while (0)

#define vtkmSOADataArrayConstructMultiComponent(dtype, ntups, ncomp, deviceAdapterId)              \
  do                                                                                               \
  {                                                                                                \
    std::vector<vtkm::cont::internal::Buffer> buffers;                                             \
    buffers.reserve(num_components);                                                               \
    for (int cc = 0; cc < num_components; ++cc)                                                    \
    {                                                                                              \
      buffers.push_back(vtkm::cont::internal::MakeBuffer(                                          \
        deviceAdapterId,                                                                           \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(num_tuples), [](void*) {},            \
        vtkm::cont::internal::InvalidRealloc));                                                    \
    }                                                                                              \
    return make_vtkmDataArray(vtkm::cont::ArrayHandleSOA<vtkm::Vec<dtype, ncomp>>(buffers));       \
  } while (0)

#define vtkmSOADataArrayCase(conduitTypeId, dtype, ntups, ncomp, deviceAdapterId)                  \
  case conduitTypeId:                                                                              \
  {                                                                                                \
    switch (num_components)                                                                        \
    {                                                                                              \
      case 1:                                                                                      \
        vtkmSOADataArrayConstructSingleComponent(dtype, num_tuples, deviceAdapterId);              \
      case 2:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 2, deviceAdapterId);            \
      case 3:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 3, deviceAdapterId);            \
      case 4:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 4, deviceAdapterId);            \
      case 5:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 5, deviceAdapterId);            \
      case 6:                                                                                      \
      default:                                                                                     \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 6, deviceAdapterId);            \
    }                                                                                              \
  }                                                                                                \
  break

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilitiesDevice::MCArrayToVTKmSOAArray(
  const conduit_node* c_mcarray, bool force_signed,
  const vtkm::cont::DeviceAdapterId& deviceAdapterId)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  const conduit_cpp::DataType dtype0 = mcarray.child(0).dtype();
  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  using conduit_dtype = conduit_cpp::DataType::Id;

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    vtkmSOADataArrayCase(
      conduit_dtype::int8, vtkm::Int8, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::int16, vtkm::Int16, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::int32, vtkm::Int32, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::int64, vtkm::Int64, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::uint8, vtkm::UInt8, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::uint16, vtkm::UInt16, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::uint32, vtkm::UInt32, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::uint64, vtkm::UInt64, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::float32, vtkm::Float32, num_tuples, num_components, deviceAdapterId);
    vtkmSOADataArrayCase(
      conduit_dtype::float64, vtkm::Float64, num_tuples, num_components, deviceAdapterId);
    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return {};
  }
  return {};
}

bool vtkConduitArrayUtilitiesDevice::IfVTKmConvertVTKMonoShapedCellArray(vtkIdType numberOfPoints,
  int cellType, vtkIdType cellSize, vtkDataArray* connectivity, vtkCellArray* cellArray)
{
  using Dispatcher = vtkArrayDispatch::DispatchByArray<internals::vtkmConnectivityArrays>;
  internals::FromDeviceConduitToMonoShapedCellArray worker{ numberOfPoints, cellSize, cellType,
    cellArray };
  return Dispatcher::Execute(connectivity, worker);
}

bool vtkConduitArrayUtilitiesDevice::IfVTKmConvertVTKMixedCellArray(vtkIdType numberOfPoints,
  vtkDataArray* offsets, vtkDataArray* shapes, vtkDataArray* elements, vtkCellArray* cellArray)
{
  using VtkmDispatcher =
    vtkArrayDispatch::Dispatch3ByArrayWithSameValueType<internals::vtkmConnectivityArrays,
      internals::vtkmConnectivityArrays, internals::vtkmConnectivityArrays>;
  internals::FromDeviceConduitToMixedCellArray deviceWorker{ numberOfPoints, cellArray };
  return VtkmDispatcher::Execute(offsets, shapes, elements, deviceWorker);
}

bool vtkConduitArrayUtilitiesDevice::CanRunOn(const vtkm::cont::DeviceAdapterId& deviceAdapterId)
{
  vtkm::cont::RuntimeDeviceTracker& runtimeTracker = vtkm::cont::GetRuntimeDeviceTracker();
  return runtimeTracker.CanRunOn(deviceAdapterId);
}

//----------------------------------------------------------------------------
void vtkConduitArrayUtilitiesDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
