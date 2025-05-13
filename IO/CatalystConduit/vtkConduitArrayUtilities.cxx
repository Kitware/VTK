// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegacy.h"

#include "vtkConduitArrayUtilities.h"
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

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
#include "vtkConduitArrayUtilitiesDevice.h"
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

  int8_t id;
  bool working;
  bool isDevicePointer = IsDevicePointer(mcarray.child(0).element_ptr(0), id, working);
  if (isDevicePointer && !working)
  {
    vtkLog(ERROR, "Viskores does not support device" + std::to_string(id));
    return nullptr;
  }
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  auto deviceAdapterId = viskores::cont::make_DeviceAdapterId(id);
#endif

  if (conduit_cpp::BlueprintMcArray::is_interleaved(mcarray))
  {
    if (isDevicePointer)
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilitiesDevice::MCArrayToVTKmAOSArray(
        conduit_cpp::c_node(&mcarray), force_signed, deviceAdapterId);
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
    if (isDevicePointer)
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilitiesDevice::MCArrayToVTKmSOAArray(
        conduit_cpp::c_node(&mcarray), force_signed, deviceAdapterId);
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
    if (isDevicePointer)
    {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
      return vtkConduitArrayUtilitiesDevice::MCArrayToVTKmSOAArray(
        conduit_cpp::c_node(&mcarray), force_signed, deviceAdapterId);
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
  int cellType, vtkIdType cellSize, const conduit_node* mcarray)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::MCArrayToVTKCellArray(
      int cellType, vtkIdType cellSize, const conduit_node* mcarray),
    "VTK 9.4",
    vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::MCArrayToVTKCellArray(
      vtkIdType numberOfPoints, int cellType, vtkIdType cellSize, const conduit_node* mcarray));
  // if arrays for the conduit nodes are stored in host memory, numberOfPoints=0 is not used
  // if arrays are stored in device memory, we'll get an error - however this case did not work
  // for the deprecated function.
  return MCArrayToVTKCellArray(0, cellType, cellSize, mcarray);
}

vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::MCArrayToVTKCellArray(
  vtkIdType numberOfPoints, int cellType, vtkIdType cellSize, const conduit_node* c_mcarray)
{
  auto connectivity =
    vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(c_mcarray, /*force_signed*/ true);
  conduit_cpp::Node mcarray = conduit_cpp::cpp_node(const_cast<conduit_node*>(c_mcarray));

  int8_t id;
  bool working;
  bool isDevicePointer = IsDevicePointer(mcarray.element_ptr(0), id, working);
  if (isDevicePointer && !working)
  {
    vtkLog(ERROR, "Viskores does not support device" + std::to_string(id));
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
  const conduit_node* c_o2mrelation, const std::string& leafname)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
      const conduit_node* c_o2mrelation, const std::string& leafname),
    "VTK 9.4",
    vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
      vtkIdType numberOfPoints, const conduit_node* c_o2mrelation, const std::string& leafname));
  // if arrays for the conduit nodes are stored in host memory, numberOfPoints=0 is not used
  // if arrays are stored in device memory, we'll get an error - however this case did not work
  // for the deprecated function.
  // leafname is always "connectivity"
  (void)leafname;
  return O2MRelationToVTKCellArray(0, c_o2mrelation);
}

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
    vtkLog(ERROR, "Viskores does not support device" + std::to_string(id));
    return nullptr;
  }

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
  vtkNew<vtkCellArray> cellArray;
  if (isDevicePointer)
  {
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
    const auto node_shapes = o2mrelation["shapes"];
    auto shapes = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
      conduit_cpp::c_node(&node_shapes), /*force_signed*/ true);
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
#if defined(VTK_USE_CUDA) || defined(VTK_KOKKOS_BACKEND_CUDA)
  cudaPointerAttributes atts;
  const cudaError_t perr = cudaPointerGetAttributes(&atts, ptr);
  // clear last error so other error checking does
  // not pick it up
  cudaError_t error = cudaGetLastError();
  bool isCudaDevice = perr == cudaSuccess &&
    (atts.type == cudaMemoryTypeDevice || atts.type == cudaMemoryTypeManaged);
  std::cerr << "Cuda device: " << isCudaDevice;
#if defined(VTK_USE_CUDA)
  id = VISKORES_DEVICE_ADAPTER_CUDA;
#elif defined(VTK_KOKKOS_BACKEND_CUDA)
  id = VISKORES_DEVICE_ADAPTER_KOKKOS;
#endif
  return isCudaDevice;
#elif defined(VTK_KOKKOS_BACKEND_HIP)
  hipPointerAttribute_t atts;
  id = VISKORES_DEVICE_ADAPTER_KOKKOS;
  const hipError_t perr = hipPointerGetAttributes(&atts, ptr);
  // clear last error so other error checking does
  // not pick it up
  hipError_t error = hipGetLastError();
  return perr == hipSuccess &&
    (atts.TYPE_ATTR == hipMemoryTypeDevice || atts.TYPE_ATTR == hipMemoryTypeUnified);
#elif defined(VTK_KOKKOS_BACKEND_SYCL)
  id = VISKORES_DEVICE_ADAPTER_KOKKOS;
#warning "SYCL device pointers are not correctly detected"
  (void)ptr;
  return false;
#else // defined(VTK_USE_CUDA) || defined(VTK_KOKKOS_BACKEND_CUDA)
  id = VISKORES_DEVICE_ADAPTER_SERIAL;
#endif
#endif // VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  (void)id;
  (void)ptr;
  return false;
}

bool vtkConduitArrayUtilities::IsDevicePointer(const void* ptr, int8_t& id, bool& working)
{
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel
  void* pointer = const_cast<void*>(ptr);
  bool isDevicePointer = vtkConduitArrayUtilities::IsDevicePointer(pointer, id);
  auto deviceAdapterId = viskores::cont::make_DeviceAdapterId(id);
  // we process host pointers using VTK which is always available
  working = isDevicePointer ? vtkConduitArrayUtilitiesDevice::CanRunOn(deviceAdapterId) : true;
#else
  void* pointer = const_cast<void*>(ptr);
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
