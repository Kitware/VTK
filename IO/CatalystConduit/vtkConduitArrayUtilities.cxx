// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConduitArrayUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSOADataArrayTemplate.h"
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

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

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

  if (conduit_cpp::BlueprintMcArray::is_interleaved(mcarray))
  {
    return vtkConduitArrayUtilities::MCArrayToVTKAOSArray(
      conduit_cpp::c_node(&mcarray), force_signed);
  }
  else if (internals::is_contiguous(mcarray))
  {
    return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
      conduit_cpp::c_node(&mcarray), force_signed);
  }
  else if (mcarray.dtype().number_of_elements() == 1)
  {
    return vtkConduitArrayUtilities::MCArrayToVTKSOAArray(
      conduit_cpp::c_node(&mcarray), force_signed);
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
  vtkIdType cellSize, const conduit_node* mcarray)
{
  auto array = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(mcarray, /*force_signed*/ true);
  if (!array)
  {
    return nullptr;
  }

  // now the array matches the type accepted by vtkCellArray (in most cases).
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(cellSize, array);
  return cellArray;
}

VTK_ABI_NAMESPACE_END

namespace
{
VTK_ABI_NAMESPACE_BEGIN

struct O2MRelationToVTKCellArrayWorker
{
  vtkNew<vtkCellArray> Cells;

  template <typename ElementsArray, typename SizesArray, typename OffsetsArray>
  void operator()(ElementsArray* elements, SizesArray* sizes, OffsetsArray* offsets)
  {
    VTK_ASSUME(elements->GetNumberOfComponents() == 1);
    VTK_ASSUME(sizes->GetNumberOfComponents() == 1);
    VTK_ASSUME(offsets->GetNumberOfComponents() == 1);

    auto& cellArray = this->Cells;
    cellArray->AllocateEstimate(offsets->GetNumberOfTuples(),
      std::max(static_cast<vtkIdType>(sizes->GetRange(0)[1]), vtkIdType(1)));

    vtkDataArrayAccessor<ElementsArray> e(elements);
    vtkDataArrayAccessor<SizesArray> s(sizes);
    vtkDataArrayAccessor<OffsetsArray> o(offsets);

    const auto numElements = sizes->GetNumberOfTuples();
    for (vtkIdType id = 0; id < numElements; ++id)
    {
      const auto offset = static_cast<vtkIdType>(o.Get(id, 0));
      const auto size = static_cast<vtkIdType>(s.Get(id, 0));

      cellArray->InsertNextCell(size);
      for (vtkIdType cc = 0; cc < size; ++cc)
      {
        cellArray->InsertCellPoint(e.Get(offset + cc, 0));
      }
    }
  }
};
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCellArray> vtkConduitArrayUtilities::O2MRelationToVTKCellArray(
  const conduit_node* c_o2mrelation, const std::string& leafname)
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

  const auto node_sizes = o2mrelation["sizes"];
  auto sizes = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
    conduit_cpp::c_node(&node_sizes), /*force_signed*/ true);
  const auto node_offsets = o2mrelation["offsets"];
  auto offsets = vtkConduitArrayUtilities::MCArrayToVTKArrayImpl(
    conduit_cpp::c_node(&node_offsets), /*force_signed*/ true);

  O2MRelationToVTKCellArrayWorker worker;

  // Using a reduced type list for typical id types.
  using TypeList =
    vtkTypeList::Unique<vtkTypeList::Create<vtkTypeInt32, vtkTypeInt64, vtkIdType>>::Result;

  using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<TypeList, TypeList, TypeList>;
  if (!Dispatcher::Execute(elements.GetPointer(), sizes.GetPointer(), offsets.GetPointer(), worker))
  {
    worker(elements.GetPointer(), sizes.GetPointer(), offsets.GetPointer());
  }

  return worker.Cells;
}

//----------------------------------------------------------------------------
void vtkConduitArrayUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
