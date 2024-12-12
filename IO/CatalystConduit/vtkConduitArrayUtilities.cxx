// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitArrayUtilities.h"

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

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
#include "vtkm/CellShape.h"
#include "vtkm/cont/ArrayHandle.h"
#include "vtkm/cont/ArrayHandleCast.h"
#include "vtkm/cont/CellSetSingleType.h"
#include "vtkm/cont/DeviceAdapterTag.h"
#include "vtkmDataArray.h"
#include "vtkmlib/CellSetConverters.h"
#if defined(VTK_USE_CUDA)
#include <cuda_runtime_api.h>
#endif // VTK_USE_CUDA
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

#include <type_traits>
#include <typeinfo>
#include <vector>

namespace internals
{
VTK_ABI_NAMESPACE_BEGIN

using AOSArrays = vtkTypeList::Unique<
  vtkTypeList::Create<vtkAOSDataArrayTemplate<vtkTypeInt8>, vtkAOSDataArrayTemplate<vtkTypeInt16>,
    vtkAOSDataArrayTemplate<vtkTypeInt32>, vtkAOSDataArrayTemplate<vtkTypeInt64>,
    vtkAOSDataArrayTemplate<vtkTypeUInt8>, vtkAOSDataArrayTemplate<vtkTypeUInt16>,
    vtkAOSDataArrayTemplate<vtkTypeUInt32>, vtkAOSDataArrayTemplate<vtkTypeUInt64>,
    vtkAOSDataArrayTemplate<vtkTypeFloat32>, vtkAOSDataArrayTemplate<vtkTypeFloat64>>>::Result;

using SOAArrays = vtkTypeList::Unique<
  vtkTypeList::Create<vtkSOADataArrayTemplate<vtkTypeInt8>, vtkSOADataArrayTemplate<vtkTypeInt16>,
    vtkSOADataArrayTemplate<vtkTypeInt32>, vtkSOADataArrayTemplate<vtkTypeInt64>,
    vtkSOADataArrayTemplate<vtkTypeUInt8>, vtkSOADataArrayTemplate<vtkTypeUInt16>,
    vtkSOADataArrayTemplate<vtkTypeUInt32>, vtkSOADataArrayTemplate<vtkTypeUInt64>,
    vtkSOADataArrayTemplate<vtkTypeFloat32>, vtkSOADataArrayTemplate<vtkTypeFloat64>>>::Result;

bool is_contiguous(const conduit_cpp::Node& node)
{
  if (node.is_contiguous())
  {
    return true;
  }
  conduit_index_t nchildren = node.number_of_children();
  for (auto i = 0; i < nchildren; ++i)
  {
    auto child = node[i];
    if (!child.is_contiguous())
    {
      return false;
    }
  }
  return true;
}

template <typename ArrayT>
vtkSmartPointer<ArrayT> CreateAOSArray(
  vtkIdType number_of_tuples, int number_of_components, const typename ArrayT::ValueType* raw_ptr)
{
  auto array = vtkSmartPointer<ArrayT>::New();
  array->SetNumberOfComponents(number_of_components);
  array->SetArray(const_cast<typename ArrayT::ValueType*>(raw_ptr),
    number_of_tuples * number_of_components, /*save=*/1);
  return array;
}

template <typename ValueT>
vtkSmartPointer<vtkSOADataArrayTemplate<ValueT>> CreateSOArray(
  vtkIdType number_of_tuples, int number_of_components, const std::vector<void*>& raw_ptrs)
{
  auto array = vtkSmartPointer<vtkSOADataArrayTemplate<ValueT>>::New();
  array->SetNumberOfComponents(number_of_components);
  for (int cc = 0; cc < number_of_components; ++cc)
  {
    array->SetArray(cc, reinterpret_cast<ValueT*>(raw_ptrs.at(cc)), number_of_tuples,
      /*updateMaxId=*/true, /*save*/ true);
  }
  return array;
}

//----------------------------------------------------------------------------
// internal: change components helper.
struct ChangeComponentsAOSImpl
{
  vtkDataArray* Input;
  template <typename ArrayT>
  void operator()(ArrayT* output)
  {
    using ValueType = typename ArrayT::ValueType;
    ArrayT* input = vtkArrayDownCast<ArrayT>(this->Input);
    const int numComps = std::max(input->GetNumberOfComponents(), output->GetNumberOfComponents());
    ValueType* tuple = new ValueType[numComps];
    std::fill(tuple, tuple + numComps, static_cast<ValueType>(0));
    for (vtkIdType cc = 0, max = input->GetNumberOfTuples(); cc < max; ++cc)
    {
      input->GetTypedTuple(cc, tuple);
      output->SetTypedTuple(cc, tuple);
    }
    delete[] tuple;
  }
};

//----------------------------------------------------------------------------
// internal: change components.
static vtkSmartPointer<vtkDataArray> ChangeComponentsAOS(vtkDataArray* array, int num_components)
{
  vtkSmartPointer<vtkDataArray> result;
  result.TakeReference(array->NewInstance());
  result->SetName(array->GetName());
  result->SetNumberOfComponents(num_components);
  result->SetNumberOfTuples(array->GetNumberOfTuples());

  ChangeComponentsAOSImpl worker{ array };
  using Dispatch = vtkArrayDispatch::DispatchByArray<AOSArrays>;
  if (!Dispatch::Execute(result, worker))
  {
    throw std::runtime_error("Failed to strip extra components from array!");
  }
  return result;
}

struct ChangeComponentsSOAImpl
{
  int Target;

  template <typename ValueT>
  void operator()(vtkSOADataArrayTemplate<ValueT>* array)
  {
    const auto numTuples = array->GetNumberOfTuples();
    const auto numComps = array->GetNumberOfComponents();
    array->SetNumberOfComponents(this->Target);

    ValueT* buffer = new ValueT[numTuples];
    std::fill_n(buffer, numTuples, 0);

    for (int cc = numComps; cc < this->Target; ++cc)
    {
      array->SetArray(cc, buffer, numTuples, /*updateMaxId=*/true,
        /*save=*/cc == numComps, /*deletMethod*/ vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
    }
  }
};

//----------------------------------------------------------------------------
static vtkSmartPointer<vtkDataArray> ChangeComponentsSOA(vtkDataArray* array, int num_components)
{
  if (array->GetNumberOfComponents() > num_components)
  {
    array->SetNumberOfComponents(num_components);
    return array;
  }

  ChangeComponentsSOAImpl worker{ num_components };
  using Dispatch = vtkArrayDispatch::DispatchByArray<SOAArrays>;
  if (!Dispatch::Execute(array, worker))
  {
    throw std::runtime_error("Failed to strip extra components from array!");
  }
  return array;
}

//----------------------------------------------------------------------------
conduit_cpp::DataType::Id GetTypeId(conduit_cpp::DataType::Id type, bool force_signed)
{
  if (!force_signed)
  {
    return type;
  }
  switch (type)
  {
    case conduit_cpp::DataType::Id::uint8:
      return conduit_cpp::DataType::Id::int8;

    case conduit_cpp::DataType::Id::uint16:
      return conduit_cpp::DataType::Id::int16;

    case conduit_cpp::DataType::Id::uint32:
      return conduit_cpp::DataType::Id::int32;

    case conduit_cpp::DataType::Id::uint64:
      return conduit_cpp::DataType::Id::int64;

    default:
      return type;
  }
}

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
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
  using inputValueType = typename ArrayT::ValueType;
  auto inputUnknownHandle = input->GetVtkmUnknownArrayHandle();
  vtkm::cont::UnknownArrayHandle connectivityUnknownHandle =
    vtkm::cont::ArrayHandle<OutputValueT>{};
  connectivityUnknownHandle.CopyShallowIfPossible(inputUnknownHandle);
  vtkm::cont::CellSetSingleType<> cellSet;
  return connectivityUnknownHandle.AsArrayHandle<vtkm::cont::ArrayHandle<OutputValueT>>();
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

//----------------------------------------------------------------------------
struct FromHostConduitToMixedCellArray
{
public:
  FromHostConduitToMixedCellArray(vtkCellArray* cellArray)
    : CellArray(cellArray)
  {
  }

  template <typename ArrayT1, typename ArrayT2>
  void operator()(ArrayT1* offsets, ArrayT2* connectivity)
  {
    // conduit offsets array does not include the last index = connectivity.size() as vtkCellArray
    vtkSmartPointer<vtkDataArray> vtkOffsets = vtk::TakeSmartPointer(offsets->NewInstance());
    vtkOffsets->SetNumberOfTuples(offsets->GetNumberOfTuples() + 1);
    vtkOffsets->SetNumberOfComponents(1);

    const auto offsetsRange = vtk::DataArrayValueRange(offsets);
    auto vtkOffsetsRange = vtk::DataArrayValueRange(vtkOffsets);
    std::copy(offsetsRange.begin(), offsetsRange.end(), vtkOffsetsRange.begin());
    *(vtkOffsetsRange.end() - 1) = connectivity->GetNumberOfTuples();
    this->CellArray->SetData(vtkOffsets, connectivity);
  }

private:
  vtkCellArray* CellArray;
};

constexpr vtkm::Int8 GetDeviceAdapterId()
{
#if defined(VTK_USE_CUDA)
  return VTKM_DEVICE_ADAPTER_CUDA;
#elif defined(VTK_KOKKOS_BACKEND_HIP)
  return VTKM_DEVICE_ADAPTER_KOKKOS;
#endif // VTK_USE_CUDA
  return VTKM_DEVICE_ADAPTER_UNDEFINED;
}
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel

VTK_ABI_NAMESPACE_END
} // internals

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkConduitArrayUtilities);
//----------------------------------------------------------------------------
vtkConduitArrayUtilities::vtkConduitArrayUtilities() = default;

//----------------------------------------------------------------------------
vtkConduitArrayUtilities::~vtkConduitArrayUtilities() = default;

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArray(
  const conduit_node* mcarray, const std::string& arrayname)
{
  if (auto array = vtkConduitArrayUtilities::MCArrayToVTKArray(mcarray))
  {
    array->SetName(arrayname.c_str());
    return array;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArray(
  const conduit_node* mcarray)
{
  return vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(mcarray, false);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCGhostArrayToVTKGhostArray(
  const conduit_node* c_mcarray, bool is_cell_data)
{
  vtkSmartPointer<vtkUnsignedCharArray> array = vtkSmartPointer<vtkUnsignedCharArray>::New();
  array->SetName(vtkDataSetAttributes::GhostArrayName());

  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));

  const int num_components = static_cast<int>(mcarray.number_of_children());
  if (num_components != 0)
  {
    vtkLogF(ERROR, "number of components for ascent_ghost should be 1 but is %d", num_components);
    return nullptr;
  }
  const conduit_cpp::DataType dtype0 = mcarray.dtype();
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());
  array->SetNumberOfTuples(num_tuples);
  const int* vals = mcarray.as_int_ptr();
  unsigned char ghost_type = is_cell_data
    ? static_cast<unsigned char>(vtkDataSetAttributes::HIDDENCELL)
    : static_cast<unsigned char>(vtkDataSetAttributes::HIDDENPOINT);
  for (vtkIdType i = 0; i < num_tuples; i++)
  {
    if (vals[i] == 0)
    {
      array->SetTypedComponent(i, 0, 0);
    }
    else
    {
      array->SetTypedComponent(i, 0, ghost_type);
    }
  }
  return array;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
  const conduit_node* c_mcarray, bool force_signed)
{
  // XXX(const-correctness): This should really be `const Node`, but is used
  // non-const in the `set_external` below.
  conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));

  conduit_cpp::Node info;
  if (!conduit_cpp::BlueprintMcArray::verify(mcarray, info))
  {
    // in some-cases, this may directly be an array of numeric values; is so, handle that.
    if (mcarray.dtype().is_number())
    {
      conduit_cpp::Node temp;
      temp.append().set_external(mcarray);
      return vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
        conduit_cpp::c_node(&temp), force_signed);
    }
    // in some cases, the array is inside a values subnode. handle that
    else if (mcarray.has_path("values"))
    {
      const auto& tmp = mcarray["values"];
      return vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
        conduit_cpp::c_node(&tmp), force_signed);
    }
    else
    {
      vtkLogF(ERROR, "invalid node of type '%s'", mcarray.dtype().name().c_str());
      return nullptr;
    }
  }

  const int number_of_components = mcarray.number_of_children();
  if (number_of_components <= 0)
  {
    vtkLogF(ERROR, "invalid number of components '%d'", number_of_components);
    return nullptr;
  }

  // confirm that all components have same type. we don't support mixed component types currently.
  // we can easily by deep copying, but we won't until needed.

  for (conduit_index_t cc = 1; cc < mcarray.number_of_children(); ++cc)
  {
    const conduit_cpp::DataType dtype0 = mcarray.child(0).dtype();
    const conduit_cpp::DataType dtypeCC = mcarray.child(cc).dtype();
    if (dtype0.id() != dtypeCC.id())
    {
      vtkLogF(ERROR,
        "mismatched component types for component 0 (%s) and %d (%s); currently not supported.",
        dtype0.name().c_str(), static_cast<int>(cc), dtypeCC.name().c_str());
      return nullptr;
    }
  }

  void* ptr = mcarray.child(0).element_ptr(0);
  if (conduit_cpp::BlueprintMcArray::is_interleaved(mcarray))
  {
    if (vtkConduitArrayUtilities::IsDevicePointer(ptr))
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilities::MCArrayToVTKmAOSArray(
        conduit_cpp::c_node(&mcarray), force_signed);
#else
      // IsDeviceMemory returns false in this case
      vtkLogF(ERROR, "VTK was not compiled with AcceleratorsVTKmDataModel");
      return nullptr;
#endif
    }
    else
    {
      return vtkConduitArrayUtilities::MCArrayToVTKAOSArray(
        conduit_cpp::c_node(&mcarray), force_signed);
    }
  }
  else if (internals::is_contiguous(mcarray))
  {
    if (vtkConduitArrayUtilities::IsDevicePointer(ptr))
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilities::MCArrayToVTKmSOAArray(
        conduit_cpp::c_node(&mcarray), force_signed);
#else
      // IsDeviceMemory returns false in this case
      vtkLogF(ERROR, "VTK was not compiled with AcceleratorsVTKmDataModel");
      return nullptr;
#endif
    }
    else
    {
      return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
        conduit_cpp::c_node(&mcarray), force_signed);
    }
  }
  else if (mcarray.dtype().number_of_elements() == 1)
  {
    if (vtkConduitArrayUtilities::IsDevicePointer(ptr))
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilities::MCArrayToVTKmSOAArray(
        conduit_cpp::c_node(&mcarray), force_signed);
#else
      // IsDeviceMemory returns false in this case
      vtkLogF(ERROR, "VTK was not compiled with AcceleratorsVTKmDataModel");
      return nullptr;
#endif
    }
    else
    {
      return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
        conduit_cpp::c_node(&mcarray), force_signed);
    }
  }
  else
  {
    // TODO: we can do a deep-copy in this case, so we can still handle it quite easily when needed.
    vtkLogF(ERROR, "unsupported array layout.");
    return nullptr;
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKAOSArray(
  const conduit_node* c_mcarray, bool force_signed)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  auto& child0 = mcarray.child(0);
  const conduit_cpp::DataType dtype0 = child0.dtype();

  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    case conduit_cpp::DataType::Id::int8:
      return internals::CreateAOSArray<vtkTypeInt8Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt8Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::int16:
      return internals::CreateAOSArray<vtkTypeInt16Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt16Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::int32:
      return internals::CreateAOSArray<vtkTypeInt32Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt32Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::int64:
      return internals::CreateAOSArray<vtkTypeInt64Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeInt64Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::uint8:
      return internals::CreateAOSArray<vtkTypeUInt8Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt8Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::uint16:
      return internals::CreateAOSArray<vtkTypeUInt16Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt16Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::uint32:
      return internals::CreateAOSArray<vtkTypeUInt32Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt32Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::uint64:
      return internals::CreateAOSArray<vtkTypeUInt64Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeUInt64Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::float32:
      return internals::CreateAOSArray<vtkTypeFloat32Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeFloat32Array::ValueType*>(child0.element_ptr(0)));

    case conduit_cpp::DataType::Id::float64:
      return internals::CreateAOSArray<vtkTypeFloat64Array>(num_tuples, num_components,
        reinterpret_cast<const vtkTypeFloat64Array::ValueType*>(child0.element_ptr(0)));

    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
  const conduit_node* c_mcarray, bool force_signed)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  const conduit_cpp::DataType dtype0 = mcarray.child(0).dtype();
  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  std::vector<void*> ptrs;
  ptrs.reserve(num_components);
  for (int cc = 0; cc < num_components; ++cc)
  {
    ptrs.push_back(const_cast<void*>(mcarray.child(cc).element_ptr(0)));
  }

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    case conduit_cpp::DataType::Id::int8:
      return internals::CreateSOArray<vtkTypeInt8>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::int16:
      return internals::CreateSOArray<vtkTypeInt16>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::int32:
      return internals::CreateSOArray<vtkTypeInt32>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::int64:
      return internals::CreateSOArray<vtkTypeInt64>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint8:
      return internals::CreateSOArray<vtkTypeUInt8>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint16:
      return internals::CreateSOArray<vtkTypeUInt16>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint32:
      return internals::CreateSOArray<vtkTypeUInt32>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint64:
      return internals::CreateSOArray<vtkTypeUInt64>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::float32:
      return internals::CreateSOArray<vtkTypeFloat32>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::float64:
      return internals::CreateSOArray<vtkTypeFloat64>(num_tuples, num_components, ptrs);

    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::SetNumberOfComponents(
  vtkDataArray* array, int num_components)
{
  if (array == nullptr || array->GetNumberOfComponents() == num_components)
  {
    return array;
  }

  if (array->HasStandardMemoryLayout())
  {
    return internals::ChangeComponentsAOS(array, num_components);
  }
  else
  {
    return internals::ChangeComponentsSOA(array, num_components);
  }
}

struct NoOp
{
  template <typename T>
  void operator()(T*)
  {
  }
};

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::MCArrayToVTKCellArray(
  vtkIdType numberOfPoints, int cellType, vtkIdType cellSize, const conduit_node* mcarray)
{
  auto connectivity =
    vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(mcarray, /*force_signed*/ true);
  if (!connectivity)
  {
    return nullptr;
  }
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  // check if cell arrays are in device memory
  using Dispatcher = vtkArrayDispatch::DispatchByArray<internals::vtkmConnectivityArrays>;
  vtkNew<vtkCellArray> cellArray;
  internals::FromDeviceConduitToMonoShapedCellArray worker{ numberOfPoints, cellSize, cellType,
    cellArray };
  if (!Dispatcher::Execute(connectivity, worker))
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  {
    // cell arrays are in host memory
    cellArray->SetData(cellSize, connectivity);
  }
  return cellArray;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
  vtkIdType numberOfPoints, const conduit_node* c_o2mrelation, const std::string& leafname)
{
  const conduit_cpp::Node o2mrelation =
    conduit_cpp::cpp_node(const_cast<conduit_node*>(c_o2mrelation));
  const auto leaf = o2mrelation[leafname];
  auto elements = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
    conduit_cpp::c_node(&leaf), /*force_signed*/ true);
  if (!elements)
  {
    return nullptr;
  }

  if (o2mrelation.has_child("indices"))
  {
    vtkLogF(WARNING, "'indices' in a O2MRelation are currently ignored.");
  }

  const auto node_offsets = o2mrelation["offsets"];
  auto offsets = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
    conduit_cpp::c_node(&node_offsets), /*force_signed*/ true);
  const auto node_shapes = o2mrelation["shapes"];
  auto shapes = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
    conduit_cpp::c_node(&node_shapes), /*force_signed*/ true);
  vtkNew<vtkCellArray> cellArray;
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  // offsets and connectivity are in device memory
  using VtkmDispatcher =
    vtkArrayDispatch::Dispatch3ByArrayWithSameValueType<internals::vtkmConnectivityArrays,
      internals::vtkmConnectivityArrays, internals::vtkmConnectivityArrays>;
  internals::FromDeviceConduitToMixedCellArray deviceWorker{ numberOfPoints, cellArray };
  if (!VtkmDispatcher::Execute(offsets, shapes, elements, deviceWorker))
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  {
    // offsets and connectivity are in host memory
    using ConduitDispatcher =
      vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Integrals>;
    internals::FromHostConduitToMixedCellArray hostWorker{ cellArray };
    if (!ConduitDispatcher::Execute(offsets, elements, hostWorker))
    {
      vtkLogF(ERROR, "offsets and elements do not have int values.");
      return nullptr;
    }
  }
  return cellArray;
}

bool vtkConduitArrayUtilities::IsDevicePointer(const void* ptr)
{
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
#if defined(VTK_USE_CUDA)
  cudaPointerAttributes atts;
  const cudaError_t perr = cudaPointerGetAttributes(&atts, ptr);
  // clear last error so other error checking does
  // not pick it up
  cudaError_t error = cudaGetLastError();
  bool isCudaDevice = perr == cudaSuccess &&
    (atts.type == cudaMemoryTypeDevice || atts.type == cudaMemoryTypeManaged);
  std::cerr << "Cuda device: " << isCudaDevice;
  return isCudaDevice;
#elif defined(VTK_KOKKOS_BACKEND_HIP)
  hipPointerAttribute_t atts;
  const hipError_t perr = hipPointerGetAttributes(&atts, ptr);
  // clear last error so other error checking does
  // not pick it up
  hipError_t error = hipGetLastError();
  return perr == hipSuccess &&
    (atts.TYPE_ATTR == hipMemoryTypeDevice || atts.TYPE_ATTR == hipMemoryTypeUnified);
#endif // VTK_USE_CUDA
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  (void)ptr;
  return false;
}

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
#define vtkmAOSDataArrayConstructSingleComponent(dtype, nvals, raw_ptr)                            \
  do                                                                                               \
  {                                                                                                \
    return make_vtkmDataArray(vtkm::cont::ArrayHandle<dtype>(                                      \
      std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(                 \
        vtkm::cont::make_DeviceAdapterId(internals::GetDeviceAdapterId()),                         \
        reinterpret_cast<dtype*>(raw_ptr), reinterpret_cast<dtype*>(raw_ptr),                      \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(nvals), [](void*) {},                 \
        vtkm::cont::internal::InvalidRealloc) }));                                                 \
  } while (0)

#define vtkmAOSDataArrayConstructMultiComponent(dtype, ntups, ncomp, raw_ptr)                      \
  do                                                                                               \
  {                                                                                                \
    return make_vtkmDataArray(vtkm::cont::ArrayHandle<vtkm::Vec<dtype, ncomp>>(                    \
      std::vector<vtkm::cont::internal::Buffer>{ vtkm::cont::internal::MakeBuffer(                 \
        vtkm::cont::make_DeviceAdapterId(internals::GetDeviceAdapterId()),                         \
        reinterpret_cast<dtype*>(raw_ptr), reinterpret_cast<dtype*>(raw_ptr),                      \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(ntups * ncomp), [](void*) {},         \
        vtkm::cont::internal::InvalidRealloc) }));                                                 \
  } while (0)

#define vtkmAOSDataArrayNumComponentsBody(dtype, ntups, ncomp, raw_ptr)                            \
  switch (num_components)                                                                          \
  {                                                                                                \
    case 1:                                                                                        \
      vtkmAOSDataArrayConstructSingleComponent(dtype, num_tuples, raw_ptr);                        \
    case 2:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 2, raw_ptr);                      \
    case 3:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 3, raw_ptr);                      \
    case 4:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 4, raw_ptr);                      \
    case 5:                                                                                        \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 5, raw_ptr);                      \
    case 6:                                                                                        \
    default:                                                                                       \
      vtkmAOSDataArrayConstructMultiComponent(dtype, num_tuples, 6, raw_ptr);                      \
  }

#define vtkmAOSDataArrayCase(conduitTypeId, dtype, ntups, ncomp, raw_ptr)                          \
  case conduitTypeId:                                                                              \
  {                                                                                                \
    vtkmAOSDataArrayNumComponentsBody(dtype, ntups, ncomp, raw_ptr);                               \
  }                                                                                                \
  break

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKmAOSArray(
  const conduit_node* c_mcarray, bool force_signed)
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
    vtkmAOSDataArrayCase(conduit_dtype::int8, vtkm::Int8, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::int16, vtkm::Int16, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::int32, vtkm::Int32, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::int64, vtkm::Int64, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::uint8, vtkm::UInt8, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::uint16, vtkm::UInt16, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::uint32, vtkm::UInt32, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(conduit_dtype::uint64, vtkm::UInt64, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(
      conduit_dtype::float32, vtkm::Float32, num_tuples, num_components, raw_ptr);
    vtkmAOSDataArrayCase(
      conduit_dtype::float64, vtkm::Float64, num_tuples, num_components, raw_ptr);
    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return {};
  }
  return {};
}

#define vtkmSOADataArrayConstructSingleComponent(dtype, nvals)                                     \
  do                                                                                               \
  {                                                                                                \
    std::vector<vtkm::cont::internal::Buffer> buffers;                                             \
    buffers.reserve(num_components);                                                               \
    for (int cc = 0; cc < num_components; ++cc)                                                    \
    {                                                                                              \
      buffers.push_back(vtkm::cont::internal::MakeBuffer(                                          \
        vtkm::cont::make_DeviceAdapterId(internals::GetDeviceAdapterId()),                         \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(nvals), [](void*) {},                 \
        vtkm::cont::internal::InvalidRealloc));                                                    \
    }                                                                                              \
    return make_vtkmDataArray(vtkm::cont::ArrayHandle<dtype>(buffers));                            \
  } while (0)

#define vtkmSOADataArrayConstructMultiComponent(dtype, ntups, ncomp)                               \
  do                                                                                               \
  {                                                                                                \
    std::vector<vtkm::cont::internal::Buffer> buffers;                                             \
    buffers.reserve(num_components);                                                               \
    for (int cc = 0; cc < num_components; ++cc)                                                    \
    {                                                                                              \
      buffers.push_back(vtkm::cont::internal::MakeBuffer(                                          \
        vtkm::cont::make_DeviceAdapterId(internals::GetDeviceAdapterId()),                         \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        reinterpret_cast<dtype*>(const_cast<void*>(mcarray.child(cc).element_ptr(0))),             \
        vtkm::internal::NumberOfValuesToNumberOfBytes<dtype>(num_tuples), [](void*) {},            \
        vtkm::cont::internal::InvalidRealloc));                                                    \
    }                                                                                              \
    return make_vtkmDataArray(vtkm::cont::ArrayHandleSOA<vtkm::Vec<dtype, ncomp>>(buffers));       \
  } while (0)

#define vtkmSOADataArrayCase(conduitTypeId, dtype, ntups, ncomp)                                   \
  case conduitTypeId:                                                                              \
  {                                                                                                \
    switch (num_components)                                                                        \
    {                                                                                              \
      case 1:                                                                                      \
        vtkmSOADataArrayConstructSingleComponent(dtype, num_tuples);                               \
      case 2:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 2);                             \
      case 3:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 3);                             \
      case 4:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 4);                             \
      case 5:                                                                                      \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 5);                             \
      case 6:                                                                                      \
      default:                                                                                     \
        vtkmSOADataArrayConstructMultiComponent(dtype, num_tuples, 6);                             \
    }                                                                                              \
  }                                                                                                \
  break

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKmSOAArray(
  const conduit_node* c_mcarray, bool force_signed)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  const conduit_cpp::DataType dtype0 = mcarray.child(0).dtype();
  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  using conduit_dtype = conduit_cpp::DataType::Id;

  switch (internals::GetTypeId(dtype0.id(), force_signed))
  {
    vtkmSOADataArrayCase(conduit_dtype::int8, vtkm::Int8, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::int16, vtkm::Int16, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::int32, vtkm::Int32, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::int64, vtkm::Int64, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::uint8, vtkm::UInt8, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::uint16, vtkm::UInt16, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::uint32, vtkm::UInt32, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::uint64, vtkm::UInt64, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::float32, vtkm::Float32, num_tuples, num_components);
    vtkmSOADataArrayCase(conduit_dtype::float64, vtkm::Float64, num_tuples, num_components);
    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return {};
  }
  return {};
}
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel

//----------------------------------------------------------------------------
void vtkConduitArrayUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
