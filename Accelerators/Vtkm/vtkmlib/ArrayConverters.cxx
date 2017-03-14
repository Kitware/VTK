//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "ArrayConverters.h"

#include "Storage.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTypedDataArray.h"

namespace tovtkm {

template <typename DataArrayType>
vtkm::cont::Field ConvertPointField(DataArrayType* input)
{
  typedef typename DataArrayType::ValueType ValueType;
  typedef
      typename tovtkm::ArrayContainerTagType<DataArrayType>::TagType TagType;
  // We know the ValueType now, so all that is left is to deduce
  // the number of components
  int numComps = input->GetNumberOfComponents();
  std::string name(input->GetName());
  switch (numComps)
  {
  case 1:
  {
    typedef ValueType VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_POINTS, dhandle);
  }
  case 2:
  {
    typedef vtkm::Vec<ValueType, 2> VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_POINTS, dhandle);
  }
  case 3:
  {
    typedef vtkm::Vec<ValueType, 3> VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_POINTS, dhandle);
  }
  case 4:
  {
    typedef vtkm::Vec<ValueType, 4> VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_POINTS, dhandle);
  }
  default:
    break;
  }
  return vtkm::cont::Field();
}

template <typename DataArrayType>
vtkm::cont::Field ConvertCellField(DataArrayType* input)
{
  typedef typename DataArrayType::ValueType ValueType;
  typedef
      typename tovtkm::ArrayContainerTagType<DataArrayType>::TagType TagType;
  // We know the ValueType now, so all that is left is to deduce
  // the number of components
  int numComps = input->GetNumberOfComponents();
  std::string name(input->GetName());

  // todo: FIX-ME
  // currently you can't get the name of a dynamic cell set so we just use
  // the default name
  // cellset.CastAndCall();
  std::string cname("cells");
  switch (numComps)
  {
  case 1:
  {
    typedef ValueType VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_CELL_SET, cname,
                             dhandle);
  }
  case 2:
  {
    typedef vtkm::Vec<ValueType, 2> VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_CELL_SET, cname,
                             dhandle);
  }
  case 3:
  {
    typedef vtkm::Vec<ValueType, 3> VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_CELL_SET, cname,
                             dhandle);
  }
  case 4:
  {
    typedef vtkm::Vec<ValueType, 4> VType;
    typedef vtkm::cont::internal::Storage<VType, TagType> StorageType;
    StorageType storage(input);
    vtkm::cont::ArrayHandle<VType, TagType> handle(storage);
    vtkm::cont::DynamicArrayHandle dhandle(handle);
    return vtkm::cont::Field(name, vtkm::cont::Field::ASSOC_CELL_SET, cname,
                             dhandle);
  }
  default:
    break;
  }
  return vtkm::cont::Field();
}

template <typename DataArrayType>
vtkm::cont::Field Convert(DataArrayType* input, int association)
{
  // we need to switch on if we are a cell or point field first!
  // The problem is that the constructor signature for fields differ based
  // on if they are a cell or point field.
  if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return ConvertPointField(input);
  }
  else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return ConvertCellField(input);
  }

  return vtkm::cont::Field();
}

// determine the type and call the proper Convert routine
vtkm::cont::Field Convert(vtkDataArray* input, int association)
{
  // The association will tell us if we have a cell or point field

  // We need to properly deduce the ValueType of the array. This means
  // that we need to switch on Float/Double/Int, and then figure out the
  // number of components. The upside is that the Convert Method can internally
  // figure out the number of components, and not have to generate a lot
  // of template to do so

  // Investigate using vtkArrayDispatch, AOS for all types, and than SOA for
  // just
  // float/double
  vtkm::cont::Field field;
  switch (input->GetDataType())
  {
    vtkTemplateMacro(
        vtkAOSDataArrayTemplate<VTK_TT>* typedIn =
            vtkAOSDataArrayTemplate<VTK_TT>::FastDownCast(input);
        if (typedIn) { field = Convert(typedIn, association); } else {
          vtkSOADataArrayTemplate<VTK_TT>* typedIn2 =
              vtkSOADataArrayTemplate<VTK_TT>::FastDownCast(input);
          if (typedIn2)
          {
            field = Convert(typedIn2, association);
          }
        });
    // end vtkTemplateMacro
  }
  return field;
}

} // namespace tovtkm

namespace fromvtkm {

namespace {

template <int Components> struct CopyArrayContents
{
  template <typename IteratorType, typename U>
  void operator()(IteratorType iter, U* array, vtkIdType numValues) const
  {
    typedef typename IteratorType::value_type T;
    // slow path for N component arrays, should be optimized once
    // the vtk soa/aos layouts get merged
    vtkIdType index = 0;
    for (vtkIdType i = 0; i < numValues; ++i, ++iter)
    {
      T t = *iter;
      for (vtkIdType j = 0; j < Components; ++j, ++index)
      {
        array->SetValue(index, t[j]);
      }
    }
  }
};

template <> struct CopyArrayContents<1>
{
  template <typename IteratorType, typename U>
  void operator()(IteratorType iter, U* array, vtkIdType numValues) const
  {
    typedef typename IteratorType::value_type T;
    // fast path for single component arrays, can't steal the memory
    // since the storage types isn't one we know
    for (vtkIdType i = 0; i < numValues; ++i, ++iter)
    {
      array->SetValue(i, *iter);
    }
  }
};

struct ArrayConverter
{
  mutable vtkDataArray* Data;

  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& handle) const
  {
    using vtkm::cont::ArrayHandle;
    using vtkm::cont::ArrayPortalToIterators;
    using vtkm::cont::ArrayPortalToIteratorBegin;

    // we need to make a copy
    using Traits = tovtkm::vtkPortalTraits<T>;
    using ValueType = typename Traits::ComponentType;
    using VTKArrayType = vtkAOSDataArrayTemplate<ValueType>;

    const vtkIdType numValues = handle.GetNumberOfValues();
    const vtkIdType numComps = Traits::NUM_COMPONENTS;
    VTKArrayType* array = VTKArrayType::New();
    array->SetNumberOfComponents(numComps);
    array->SetNumberOfTuples(numValues);

    // copy directly into the vtkarray
    typedef vtkm::cont::ArrayHandle<T, S> HandleType;
    typedef ArrayPortalToIterators<typename HandleType::PortalConstControl>
        PortalToIter;

    PortalToIter iterators(handle.GetPortalConstControl());
    typename PortalToIter::IteratorType iter = iterators.GetBegin();

    // Need to do a compile time switch for copying a single component
    // array versus a multiple component array
    CopyArrayContents<Traits::NUM_COMPONENTS> copier;
    copier(iter, array, numValues);
    this->Data = array;
  }

  template <typename T>
  void operator()(
      const vtkm::cont::ArrayHandle<T,
                                    vtkm::cont::StorageTagBasic>& handle) const
  {
    // we can steal this array!
    using Traits = tovtkm::vtkPortalTraits<T>;
    using ValueType = typename Traits::ComponentType;
    using VTKArrayType = vtkAOSDataArrayTemplate<ValueType>;

    const vtkm::Id size = handle.GetNumberOfValues() * Traits::NUM_COMPONENTS;

    VTKArrayType* array = VTKArrayType::New();
    array->SetNumberOfComponents(Traits::NUM_COMPONENTS);

    handle.SyncControlArray();
    ValueType* stolenMemory = reinterpret_cast<ValueType*>(
        handle.Internals->ControlArray.StealArray());

    //VTK-m allocations are all aligned
    array->SetVoidArray(stolenMemory, size, 0,
                        vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE);

    this->Data = array;
  }

  template <typename T>
  void operator()(
      const vtkm::cont::ArrayHandle<T, tovtkm::vtkAOSArrayContainerTag>& handle)
      const
  {
    // we can grab the already allocated vtk memory
    this->Data = handle.Internals->ControlArray.VTKArray();
    this->Data->Register(NULL);
  }

  template <typename T>
  void operator()(
      const vtkm::cont::ArrayHandle<T, tovtkm::vtkSOAArrayContainerTag>& handle)
      const
  {
    // we can grab the already allocated vtk memory
    this->Data = handle.Internals->ControlArray.VTKArray();
    this->Data->Register(NULL);
  }
};
}

vtkDataArray* Convert(const vtkm::cont::Field& input)
{
  // We need to do the conversion from Field to a known vtkm::cont::ArrayHandle
  // after that we need to fill the vtkDataArray
  vtkmOutputFilterPolicy policy;
  vtkDataArray* data = nullptr;
  ArrayConverter aConverter;

  try
  {
    vtkm::filter::ApplyPolicy(input, policy).CastAndCall(aConverter);
    data = aConverter.Data;
    if (data)
    {
      data->SetName(input.GetName().c_str());
    }
  }
  catch (vtkm::cont::Error&)
  {
  }
  return data;
}

vtkPoints* Convert(const vtkm::cont::CoordinateSystem& input)
{
  // We need to do the conversion from Field to a known vtkm::cont::ArrayHandle
  // after that we need to fill the vtkDataArray
  vtkmOutputFilterPolicy policy;
  ArrayConverter aConverter;

  vtkPoints* points = nullptr;
  try
  {
    vtkm::filter::ApplyPolicy(input, policy).CastAndCall(aConverter);
    vtkDataArray* pdata = aConverter.Data;
    points = vtkPoints::New();
    points->SetData(pdata);
    pdata->FastDelete();
  }
  catch (vtkm::cont::Error &e)
  {
    vtkGenericWarningMacro("Converting vtkm::cont::CoordinateSystem to "
                           "vtkPoints failed: " << e.what());
  }
  return points;
}

bool ConvertArrays(const vtkm::cont::DataSet& input, vtkDataSet* output)
{
  vtkPointData* pd = output->GetPointData();
  vtkCellData* cd = output->GetCellData();

  vtkm::IdComponent numFields = input.GetNumberOfFields();
  for (vtkm::IdComponent i = 0; i < numFields; ++i)
  {
    const vtkm::cont::Field& f = input.GetField(i);
    vtkDataArray* vfield = Convert(f);
    if (vfield && f.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS)
    {
      pd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if (vfield &&  f.GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET)
    {
      cd->AddArray(vfield);
      vfield->FastDelete();
    }
    else if(vfield)
    {
      vfield->Delete();
    }
  }
  return true;
}
}
