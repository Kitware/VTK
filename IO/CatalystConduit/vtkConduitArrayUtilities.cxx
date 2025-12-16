// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegacy.h"

#include "vtkConduitArrayUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSOATypeFloat32Array.h"
#include "vtkSOATypeFloat64Array.h"
#include "vtkSOATypeInt16Array.h"
#include "vtkSOATypeInt32Array.h"
#include "vtkSOATypeInt64Array.h"
#include "vtkSOATypeInt8Array.h"
#include "vtkSOATypeUInt16Array.h"
#include "vtkSOATypeUInt32Array.h"
#include "vtkSOATypeUInt64Array.h"
#include "vtkSOATypeUInt8Array.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStridedTypeFloat32Array.h"
#include "vtkStridedTypeFloat64Array.h"
#include "vtkStridedTypeInt16Array.h"
#include "vtkStridedTypeInt32Array.h"
#include "vtkStridedTypeInt64Array.h"
#include "vtkStridedTypeInt8Array.h"
#include "vtkStridedTypeUInt16Array.h"
#include "vtkStridedTypeUInt32Array.h"
#include "vtkStridedTypeUInt64Array.h"
#include "vtkStridedTypeUInt8Array.h"
#include "vtkStringFormatter.h"
#include "vtkType.h"
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
#include "vtkConduitArrayUtilitiesDevice.h"
#include "vtkmDataArrayUtilities.h"
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

template <typename AOSArrayT, typename ValueType = typename AOSArrayT::ValueType>
vtkSmartPointer<AOSArrayT> CreateAOSArray(
  vtkIdType number_of_tuples, int number_of_components, const void* raw_ptr)
{
  auto array = vtkSmartPointer<AOSArrayT>::New();
  array->SetNumberOfComponents(number_of_components);
  array->SetArray(static_cast<ValueType*>(const_cast<void*>(raw_ptr)),
    number_of_tuples * number_of_components, /*save=*/1);
  return array;
}

template <typename StridedArrayT, typename ValueType = typename StridedArrayT::ValueType>
vtkSmartPointer<StridedArrayT> CreateStridedArray(const conduit_cpp::Node& arrayNode)
{
  const auto& child0 = arrayNode.child(0);
  const conduit_cpp::DataType dtype0 = child0.dtype();
  const int number_of_components = static_cast<int>(arrayNode.number_of_children());
  const vtkIdType number_of_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());
  const int strideBytes = dtype0.stride();
  const int index_stride = strideBytes / (sizeof(ValueType));

  auto array = vtkSmartPointer<StridedArrayT>::New();
  array->SetNumberOfComponents(number_of_components);
  array->SetNumberOfTuples(number_of_tuples);
  array->ConstructBackend(
    static_cast<const ValueType*>(child0.element_ptr(0)), index_stride, number_of_components);
  return array;
}

template <typename SOAArrayT, typename ValueType = typename SOAArrayT::ValueType>
vtkSmartPointer<SOAArrayT> CreateSOArray(
  vtkIdType number_of_tuples, int number_of_components, const std::vector<void*>& raw_ptrs)
{
  auto array = vtkSmartPointer<SOAArrayT>::New();
  array->SetNumberOfComponents(number_of_components);
  for (int cc = 0; cc < number_of_components; ++cc)
  {
    array->SetArray(cc, static_cast<ValueType*>(raw_ptrs.at(cc)), number_of_tuples,
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
    auto vtkOffsets = vtk::TakeSmartPointer(offsets->NewInstance());
    vtkOffsets->SetNumberOfTuples(offsets->GetNumberOfTuples() + 1);
    vtkOffsets->SetNumberOfComponents(1);

    const auto offsetsRange = vtk::DataArrayValueRange<1>(offsets);
    auto vtkOffsetsRange = vtk::DataArrayValueRange<1>(vtkOffsets);
    std::copy(offsetsRange.begin(), offsetsRange.end(), vtkOffsetsRange.begin());
    *(vtkOffsetsRange.end() - 1) = connectivity->GetNumberOfTuples();
    this->CellArray->SetData(vtkOffsets, connectivity);
  }

private:
  vtkCellArray* CellArray;
};

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
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKArray(
  const conduit_node* c_mcarray)
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
      return vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&temp));
    }
    // in some cases, the array is inside a values subnode. handle that
    else if (mcarray.has_path("values"))
    {
      const auto& tmp = mcarray["values"];
      return vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&tmp));
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

  int8_t id;
  bool working;
  bool isDevicePointer = IsDevicePointer(mcarray.child(0).element_ptr(0), id, working);
  if (isDevicePointer && !working)
  {
    vtkLog(ERROR, "Viskores does not support device" + vtk::to_string(id));
    return nullptr;
  }
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  auto deviceAdapterId = viskores::cont::make_DeviceAdapterId(id);
#endif

  auto numTuples = mcarray.child(0).dtype().number_of_elements();
  if (conduit_cpp::BlueprintMcArray::is_interleaved(mcarray) || numTuples == 0)
  {
    if (isDevicePointer)
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilitiesDevice::MCArrayToVTKmAOSArray(
        conduit_cpp::c_node(&mcarray), deviceAdapterId);
#else
      // IsDeviceMemory returns false in this case
      vtkLogF(ERROR, "VTK was not compiled with AcceleratorsVTKmDataModel");
      return nullptr;
#endif
    }
    else
    {
      const auto& child0 = mcarray.child(0);
      const conduit_cpp::DataType dtype0 = child0.dtype();

      if (mcarray.number_of_children() * dtype0.element_bytes() != dtype0.stride())
      {
        // there is some data interlaced with current array
        return vtkConduitArrayUtilities::MCArrayToVTKStridedArray(conduit_cpp::c_node(&mcarray));
      }

      return vtkConduitArrayUtilities::MCArrayToVTKAOSArray(conduit_cpp::c_node(&mcarray));
    }
  }
  else if (internals::is_contiguous(mcarray))
  {
    if (isDevicePointer)
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilitiesDevice::MCArrayToVTKmSOAArray(
        conduit_cpp::c_node(&mcarray), deviceAdapterId);
#else
      // IsDeviceMemory returns false in this case
      vtkLogF(ERROR, "VTK was not compiled with AcceleratorsVTKmDataModel");
      return nullptr;
#endif
    }
    else
    {
      return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(conduit_cpp::c_node(&mcarray));
    }
  }
  else if (mcarray.dtype().number_of_elements() == 1)
  {
    if (isDevicePointer)
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilitiesDevice::MCArrayToVTKmSOAArray(
        conduit_cpp::c_node(&mcarray), deviceAdapterId);
#else
      // IsDeviceMemory returns false in this case
      vtkLogF(ERROR, "VTK was not compiled with AcceleratorsVTKmDataModel");
      return nullptr;
#endif
    }
    else
    {
      return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(conduit_cpp::c_node(&mcarray));
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
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKStridedArray(
  const conduit_node* c_mcarray)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  const auto& child0 = mcarray.child(0);
  const conduit_cpp::DataType dtype0 = child0.dtype();

  switch (dtype0.id())
  {
    case conduit_cpp::DataType::Id::int8:
      return internals::CreateStridedArray<vtkStridedTypeInt8Array>(mcarray);

    case conduit_cpp::DataType::Id::int16:
      return internals::CreateStridedArray<vtkStridedTypeInt16Array>(mcarray);

    case conduit_cpp::DataType::Id::int32:
      return internals::CreateStridedArray<vtkStridedTypeInt32Array>(mcarray);

    case conduit_cpp::DataType::Id::int64:
      return internals::CreateStridedArray<vtkStridedTypeInt64Array>(mcarray);

    case conduit_cpp::DataType::Id::uint8:
      return internals::CreateStridedArray<vtkStridedTypeUInt8Array>(mcarray);

    case conduit_cpp::DataType::Id::uint16:
      return internals::CreateStridedArray<vtkStridedTypeUInt16Array>(mcarray);

    case conduit_cpp::DataType::Id::uint32:
      return internals::CreateStridedArray<vtkStridedTypeUInt32Array>(mcarray);

    case conduit_cpp::DataType::Id::uint64:
      return internals::CreateStridedArray<vtkStridedTypeUInt64Array>(mcarray);

    case conduit_cpp::DataType::Id::float32:
      return internals::CreateStridedArray<vtkStridedTypeFloat32Array>(mcarray);

    case conduit_cpp::DataType::Id::float64:
      return internals::CreateStridedArray<vtkStridedTypeFloat64Array>(mcarray);

    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKAOSArray(
  const conduit_node* c_mcarray)
{
  const conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));
  auto& child0 = mcarray.child(0);
  const conduit_cpp::DataType dtype0 = child0.dtype();

  const int num_components = static_cast<int>(mcarray.number_of_children());
  const vtkIdType num_tuples = static_cast<vtkIdType>(dtype0.number_of_elements());

  switch (dtype0.id())
  {
    case conduit_cpp::DataType::Id::int8:
      return internals::CreateAOSArray<vtkTypeInt8Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::int16:
      return internals::CreateAOSArray<vtkTypeInt16Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::int32:
      return internals::CreateAOSArray<vtkTypeInt32Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::int64:
      return internals::CreateAOSArray<vtkTypeInt64Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::uint8:
      return internals::CreateAOSArray<vtkTypeUInt8Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::uint16:
      return internals::CreateAOSArray<vtkTypeUInt16Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::uint32:
      return internals::CreateAOSArray<vtkTypeUInt32Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::uint64:
      return internals::CreateAOSArray<vtkTypeUInt64Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::float32:
      return internals::CreateAOSArray<vtkTypeFloat32Array>(
        num_tuples, num_components, child0.element_ptr(0));

    case conduit_cpp::DataType::Id::float64:
      return internals::CreateAOSArray<vtkTypeFloat64Array>(
        num_tuples, num_components, child0.element_ptr(0));

    default:
      vtkLogF(ERROR, "unsupported data type '%s' ", dtype0.name().c_str());
      return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
  const conduit_node* c_mcarray)
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

  switch (dtype0.id())
  {
    case conduit_cpp::DataType::Id::int8:
      return internals::CreateSOArray<vtkSOATypeInt8Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::int16:
      return internals::CreateSOArray<vtkSOATypeInt16Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::int32:
      return internals::CreateSOArray<vtkSOATypeInt32Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::int64:
      return internals::CreateSOArray<vtkSOATypeInt64Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint8:
      return internals::CreateSOArray<vtkSOATypeUInt8Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint16:
      return internals::CreateSOArray<vtkSOATypeUInt16Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint32:
      return internals::CreateSOArray<vtkSOATypeUInt32Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::uint64:
      return internals::CreateSOArray<vtkSOATypeUInt64Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::float32:
      return internals::CreateSOArray<vtkSOATypeFloat32Array>(num_tuples, num_components, ptrs);

    case conduit_cpp::DataType::Id::float64:
      return internals::CreateSOArray<vtkSOATypeFloat64Array>(num_tuples, num_components, ptrs);

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
  vtkIdType numberOfPoints, int cellType, vtkIdType cellSize, const conduit_node* c_mcarray)
{
  auto connectivity = vtkConduitArrayUtilities::MCArrayToVTKArray(c_mcarray);
  conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));

  int8_t id;
  bool working;
  bool isDevicePointer = IsDevicePointer(mcarray.element_ptr(0), id, working);
  if (isDevicePointer && !working)
  {
    vtkLog(ERROR, "Viskores does not support device" + vtk::to_string(id));
    return nullptr;
  }
  if (!connectivity)
  {
    return nullptr;
  }
  vtkNew<vtkCellArray> cellArray;
  if (isDevicePointer)
  {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
    if (!vtkConduitArrayUtilitiesDevice::IfVTKmConvertVTKMonoShapedCellArray(
          numberOfPoints, cellType, cellSize, connectivity, cellArray))
    {
      vtkLogF(ERROR, "Cannot convert connectivity to a vtkmArray");
      return nullptr;
    }
#else
    (void)cellType;
    (void)numberOfPoints; // avoid unused variable warning
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  }
  else
  {
    // cell arrays are in host memory
    cellArray->SetData(cellSize, connectivity);
  }
  return cellArray;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
  vtkIdType numberOfPoints, const conduit_node* c_o2mrelation)
{
  const conduit_cpp::Node o2mrelation =
    conduit_cpp::cpp_node(const_cast<conduit_node*>(c_o2mrelation));
  const auto leaf = o2mrelation["connectivity"];

  int8_t id;
  bool working;
  bool isDevicePointer = IsDevicePointer(leaf.element_ptr(0), id, working);
  if (isDevicePointer && !working)
  {
    vtkLog(ERROR, "Viskores does not support device" + vtk::to_string(id));
    return nullptr;
  }

  auto elements = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&leaf));
  if (!elements)
  {
    return nullptr;
  }

  if (o2mrelation.has_child("indices"))
  {
    vtkLogF(WARNING, "'indices' in a O2MRelation are currently ignored.");
  }

  const auto node_offsets = o2mrelation["offsets"];
  auto offsets = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&node_offsets));
  vtkNew<vtkCellArray> cellArray;
  if (isDevicePointer)
  {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
    const auto node_shapes = o2mrelation["shapes"];
    auto shapes = vtkConduitArrayUtilities::MCArrayToVTKArray(conduit_cpp::c_node(&node_shapes));
    if (!vtkConduitArrayUtilitiesDevice::IfVTKmConvertVTKMixedCellArray(
          numberOfPoints, offsets, shapes, elements, cellArray))
    {
      vtkLogF(ERROR, "Cannot convert connectivity to a vtkmArray");
      return nullptr;
    }
#else
    (void)numberOfPoints; // avoid unused variable warning
#endif
  }
  else
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

/**
 * Returns true if the pointer is in device memory. In that case
 * id is the DeviceAdapterTag
 */
bool vtkConduitArrayUtilities::IsDevicePointer(const void* ptr, int8_t& id)
{
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  return vtkmDataArrayUtilities::IsDevicePointer(ptr, id);
#else
  (void)id;
  (void)ptr;
  return false;
#endif
}

bool vtkConduitArrayUtilities::IsDevicePointer(const void* ptr, int8_t& id, bool& working)
{
  void* pointer = const_cast<void*>(ptr);
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  bool isDevicePointer = vtkConduitArrayUtilities::IsDevicePointer(pointer, id);
  // we process host pointers using VTK which is always available
  working = isDevicePointer ? vtkmDataArrayUtilities::IsDeviceAdapterAvailable(id) : true;
#else
  bool isDevicePointer = vtkConduitArrayUtilities::IsDevicePointer(pointer, id);
  // no Viskores, so for a device pointer there is no runtime
  // for host pointer VTK can handle that.
  working = (isDevicePointer) ? false : true;
#endif
  return isDevicePointer;
}

//----------------------------------------------------------------------------
void vtkConduitArrayUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
