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

#ifndef vtkmlib_Storage_h
#define vtkmlib_Storage_h

#include "Portals.h"
#include "vtkmTags.h"
#include <vtkm/cont/Storage.h>

namespace vtkm {
namespace cont {
namespace internal {

template <typename ValueType_>
class VTKM_ALWAYS_EXPORT Storage<ValueType_, tovtkm::vtkAOSArrayContainerTag>
{
  static const int NUM_COMPONENTS = tovtkm::vtkPortalTraits<ValueType_>::NUM_COMPONENTS;
  typedef typename tovtkm::vtkPortalTraits<ValueType_>::ComponentType ComponentType;

public:
  typedef ValueType_ ValueType;
  typedef vtkAOSDataArrayTemplate<ComponentType> ArrayType;
  typedef tovtkm::vtkArrayPortal<ValueType, ArrayType> PortalType;
  typedef tovtkm::vtkArrayPortal<const ValueType, ArrayType> PortalConstType;

  Storage()
    : Array(NULL),
      NumberOfValues(0),
      AllocatedSize(0),
      DeallocateOnRelease(false),
      UserProvidedMemory(false)
  {
  }

  Storage(vtkAOSDataArrayTemplate<ComponentType>* array)
    : Array(array), // NumberOfValues should be equal to NumberOfTuples
      // all the logic in here is busted
      NumberOfValues(array->GetNumberOfTuples()),
      AllocatedSize(array->GetNumberOfTuples() * NUM_COMPONENTS),
      DeallocateOnRelease(false),
      UserProvidedMemory(true)
  {
  }

  ~Storage()
  {
    this->ReleaseResources();
  }

  Storage&
  operator=(const Storage<ValueType, tovtkm::vtkAOSArrayContainerTag>& src)
  {
    if (src.DeallocateOnRelease)
    {
      throw vtkm::cont::ErrorBadValue(
          "Attempted to copy a storage array that needs deallocation. "
          "This is disallowed to prevent complications with deallocation.");
    }

    this->ReleaseResources();
    this->Array = src.Array;
    this->NumberOfValues = src.NumberOfValues;
    this->AllocatedSize = src.AllocatedSize;
    this->DeallocateOnRelease = src.DeallocateOnRelease;
    this->UserProvidedMemory = src.UserProvidedMemory;

    return *this;
  }

  void ReleaseResources();

  void Allocate(vtkm::Id numberOfValues);

  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(vtkm::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
          "Shrink method cannot be used to grow array.");
    }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal();

  PortalConstType GetPortalConst() const;

  ArrayType* VTKArray() const
  {
    return this->Array;
  }

private:
  ArrayType* Array;
  vtkm::Id NumberOfValues;
  vtkm::Id AllocatedSize;
  bool DeallocateOnRelease;
  bool UserProvidedMemory;
};

template <typename ValueType_>
class VTKM_ALWAYS_EXPORT Storage<ValueType_, tovtkm::vtkSOAArrayContainerTag>
{
  static const int NUM_COMPONENTS = tovtkm::vtkPortalTraits<ValueType_>::NUM_COMPONENTS;
  typedef typename tovtkm::vtkPortalTraits<ValueType_>::ComponentType ComponentType;

public:
  typedef ValueType_ ValueType;

  typedef vtkSOADataArrayTemplate<ComponentType> ArrayType;
  typedef tovtkm::vtkArrayPortal<ValueType, ArrayType> PortalType;
  typedef tovtkm::vtkArrayPortal<const ValueType, ArrayType> PortalConstType;

  Storage()
    : Array(NULL),
      NumberOfValues(0),
      AllocatedSize(0),
      DeallocateOnRelease(false),
      UserProvidedMemory(false)
  {
  }

  Storage(vtkSOADataArrayTemplate<ComponentType>* array)
    : Array(array),
      NumberOfValues(array->GetNumberOfTuples()),
      AllocatedSize(array->GetNumberOfTuples() * NUM_COMPONENTS),
      DeallocateOnRelease(false),
      UserProvidedMemory(true)
  {
  }

  ~Storage()
  {
    this->ReleaseResources();
  }

  Storage&
  operator=(const Storage<ValueType_, tovtkm::vtkSOAArrayContainerTag>& src)
  {
    if (src.DeallocateOnRelease)
    {
      throw vtkm::cont::ErrorBadValue(
          "Attempted to copy a storage array that needs deallocation. "
          "This is disallowed to prevent complications with deallocation.");
    }

    this->ReleaseResources();
    this->Array = src.Array;
    this->NumberOfValues = src.NumberOfValues;
    this->AllocatedSize = src.AllocatedSize;
    this->DeallocateOnRelease = src.DeallocateOnRelease;
    this->UserProvidedMemory = src.UserProvidedMemory;

    return *this;
  }

  void ReleaseResources();

  void Allocate(vtkm::Id numberOfValues);

  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(vtkm::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
          "Shrink method cannot be used to grow array.");
    }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal();

  PortalConstType GetPortalConst() const;

  ArrayType* VTKArray() const
  {
    return this->Array;
  }

private:
  ArrayType* Array;
  vtkm::Id NumberOfValues;
  vtkm::Id AllocatedSize;
  bool DeallocateOnRelease;
  bool UserProvidedMemory;
};

template <typename ValueType_>
class VTKM_ALWAYS_EXPORT Storage<ValueType_, tovtkm::vtkCellArrayContainerTag>
{
public:
  typedef ValueType_ ValueType;
  // construct the portals type to be used with this container
  typedef vtkAOSDataArrayTemplate<ValueType_> ArrayType;
  typedef tovtkm::vtkArrayPortal<ValueType, ArrayType> PortalType;
  typedef tovtkm::vtkArrayPortal<const ValueType, ArrayType> PortalConstType;

  Storage()
    : Array(NULL),
      NumberOfValues(0),
      AllocatedSize(0),
      DeallocateOnRelease(false),
      UserProvidedMemory(false)
  {
  }

  Storage(vtkCellArray* array)
    : Array(array),
      NumberOfValues(array->GetNumberOfConnectivityEntries()),
      AllocatedSize(array->GetSize()),
      DeallocateOnRelease(false),
      UserProvidedMemory(true)
  {
  }

  ~Storage()
  {
    this->ReleaseResources();
  }

  Storage&
  operator=(const Storage<ValueType_, tovtkm::vtkCellArrayContainerTag>& src)
  {
    if (src.DeallocateOnRelease)
    {
      throw vtkm::cont::ErrorBadValue(
          "Attempted to copy a storage array that needs deallocation. "
          "This is disallowed to prevent complications with deallocation.");
    }

    this->ReleaseResources();
    this->Array = src.Array;
    this->NumberOfValues = src.NumberOfValues;
    this->AllocatedSize = src.AllocatedSize;
    this->DeallocateOnRelease = src.DeallocateOnRelease;
    this->UserProvidedMemory = src.UserProvidedMemory;

    return *this;
  }

  void ReleaseResources();

  void Allocate(vtkm::Id numberOfValues);

  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(vtkm::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
          "Shrink method cannot be used to grow array.");
    }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal();

  PortalConstType GetPortalConst() const;

  vtkCellArray* VTKArray() const
  {
    return this->Array;
  }

private:
  vtkCellArray* Array;
  vtkm::Id NumberOfValues;
  vtkm::Id AllocatedSize;
  bool DeallocateOnRelease;
  bool UserProvidedMemory;
};
}
}
}

#define VTKM_TEMPLATE_EXPORT_Storage(T, S)                                     \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<const T, S>;        \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<T, S>;              \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      Storage<const vtkm::Vec<T, 2>, S>;                                       \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      Storage<vtkm::Vec<T, 2>, S>;                                             \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      Storage<const vtkm::Vec<T, 3>, S>;                                       \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      Storage<vtkm::Vec<T, 3>, S>;                                             \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      Storage<const vtkm::Vec<T, 4>, S>;                                       \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<vtkm::Vec<T, 4>, S>;

#define VTKM_TEMPLATE_IMPORT_Storage(T, S)                                     \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<const T, S>;               \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<T, S>;                     \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<const vtkm::Vec<T, 2>, S>; \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<vtkm::Vec<T, 2>, S>;       \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<const vtkm::Vec<T, 3>, S>; \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<vtkm::Vec<T, 3>, S>;       \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<const vtkm::Vec<T, 4>, S>; \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT Storage<vtkm::Vec<T, 4>, S>;

#ifndef vtkmlib_Storage_cxx
namespace vtkm {
namespace cont {
namespace internal {
// T extern template instantiations
VTKM_TEMPLATE_EXPORT_Storage(char, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int8, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt8, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int16, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt16, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int32, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt32, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int64, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt64, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Float32, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Float64, tovtkm::vtkAOSArrayContainerTag);

VTKM_TEMPLATE_EXPORT_Storage(char, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int8, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt8, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int16, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt16, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int32, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt32, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Int64, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::UInt64, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Float32, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(vtkm::Float64, tovtkm::vtkSOAArrayContainerTag);

#if VTKM_SIZE_LONG_LONG == 8
VTKM_TEMPLATE_EXPORT_Storage(long, tovtkm::vtkAOSArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(unsigned long, tovtkm::vtkAOSArrayContainerTag);

VTKM_TEMPLATE_EXPORT_Storage(long, tovtkm::vtkSOAArrayContainerTag);
VTKM_TEMPLATE_EXPORT_Storage(unsigned long, tovtkm::vtkSOAArrayContainerTag);
#endif

extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
    Storage<vtkIdType, tovtkm::vtkCellArrayContainerTag>;
}
}
}

#endif // defined vtkmlib_Storage_cxx

#include "Storage.hxx"
#endif // vtkmlib_Storage_h
