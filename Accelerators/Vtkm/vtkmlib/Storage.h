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
class VTKM_ALWAYS_EXPORT Storage<ValueType_, tovtkm::vtkCellArrayContainerTag>
{
public:
  typedef ValueType_ ValueType;
  // construct the portals type to be used with this container
  typedef vtkAOSDataArrayTemplate<ValueType_> ArrayType;
  typedef tovtkm::vtkArrayPortal<ValueType, ArrayType> PortalType;
  typedef tovtkm::vtkArrayPortal<const ValueType, ArrayType> PortalConstType;

  Storage()
    : Array(nullptr),
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

#ifndef vtkmlib_Storage_cxx
namespace vtkm {
namespace cont {
namespace internal {
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
    Storage<vtkIdType, tovtkm::vtkCellArrayContainerTag>;
}
}
}

#endif // defined vtkmlib_Storage_cxx

#include "Storage.hxx"
#endif // vtkmlib_Storage_h
