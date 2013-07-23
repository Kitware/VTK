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

#ifndef vtkToDax_Containers_h
#define vtkToDax_Containers_h

#include "vtkPoints.h"
#include "vtkCellArray.h"

#include "Portals.h"
#include "Allocators.h"

namespace vtkToDax {

//tag to say we are creating an array container
//of type vtkIdTypeDataArray, vtkFloatDataArray, etc
template<typename VTKArrayType>
struct vtkArrayContainerTag
{
  typedef VTKArrayType Type;
};

//this tag is used to construct points coordinates
struct vtkPointsContainerTag
{
};

//this tag is used to construct a vtkIdArray that is
//used for cells
template<typename CellType>
struct vtkTopologyContainerTag
{
  typedef CellType Type;
};
}

namespace dax {
namespace cont {
namespace internal {


template <typename ValueT, typename VTKArrayType>
class ArrayContainerControl<ValueT,vtkToDax::vtkArrayContainerTag<VTKArrayType> >
{
public:
  typedef ValueT ValueType;
  typedef vtkToDax::vtkArrayPortal<ValueType> PortalType;
  typedef vtkToDax::vtkArrayPortal<const ValueType> PortalConstType;

private:
  //determine the number of components we need to allocate in the vtkArray
  static const int NUM_COMPONENTS = dax::VectorTraits<ValueType>::NUM_COMPONENTS;

  //construct the allocator with the right number of elements
  typedef vtkToDax::vtkAlloc<VTKArrayType,NUM_COMPONENTS> AllocatorType;

  //the allocated type from the allocator
  typedef typename AllocatorType::pointer PointerType;

public:

  ArrayContainerControl() : Array(NULL), NumberOfValues(0) { }

  ~ArrayContainerControl()
  {
    this->ReleaseResources();
  }

  void ReleaseResources()
  {
    if (this->NumberOfValues > 0)
      {
      DAX_ASSERT_CONT(this->Array != NULL);
      AllocatorType allocator;
      allocator.deallocate(this->Array, this->NumberOfValues);
      this->Array = NULL;
      this->NumberOfValues = 0;
      }
    else
      {
      DAX_ASSERT_CONT(this->Array == NULL);
      }
  }

  void Allocate(dax::Id numberOfValues)
  {
    if (this->NumberOfValues == numberOfValues) return;

    this->ReleaseResources();
    try
      {
      if (numberOfValues > 0)
        {
        AllocatorType allocator;
        this->Array = allocator.allocate(numberOfValues);
        this->NumberOfValues = numberOfValues;
        }
      else
        {
        // ReleaseResources should have already set NumberOfValues to 0.
        DAX_ASSERT_CONT(this->NumberOfValues == 0);
        }
      }
    catch (std::bad_alloc err)
      {
      // Make sureour state is OK.
      this->Array = NULL;
      this->NumberOfValues = 0;
      throw dax::cont::ErrorControlOutOfMemory(
            "Could not allocate basic control array.");
      }
  }

  dax::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(dax::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
      {
      throw dax::cont::ErrorControlBadValue(
            "Shrink method cannot be used to grow array.");
      }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal()
  {
    return PortalType(this->Array, this->NumberOfValues);
  }

  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array, this->NumberOfValues);
  }

private:
  // Not implemented.
  ArrayContainerControl(const ArrayContainerControl<ValueType, vtkToDax::vtkPointsContainerTag> &src);
  void operator=(const ArrayContainerControl<ValueType, vtkToDax::vtkPointsContainerTag> &src);

  PointerType Array;
  dax::Id NumberOfValues;
};

template <typename ValueT>
class ArrayContainerControl<ValueT,vtkToDax::vtkPointsContainerTag>
{
public:
  typedef ValueT ValueType;
  //construct the portals type to be used with this container
  typedef vtkToDax::vtkPointsPortal<ValueType> PortalType;
  typedef vtkToDax::vtkPointsPortal<const ValueType> PortalConstType;

private:
  //determine the allocator type and pointer type for this container
  typedef vtkToDax::vtkAlloc<vtkPoints,3> AllocatorType;
  //the pointer type tells us the type of what the allocator returns
  typedef typename AllocatorType::pointer PointerType;

public:

  ArrayContainerControl() : Array(NULL), NumberOfValues(0) { }

  ~ArrayContainerControl()
  {
    this->ReleaseResources();
  }

  void ReleaseResources()
  {
    if (this->NumberOfValues > 0)
      {
      DAX_ASSERT_CONT(this->Array != NULL);
      AllocatorType allocator;
      allocator.deallocate(this->Array, this->NumberOfValues);
      this->Array = NULL;
      this->NumberOfValues = 0;
      }
    else
      {
      DAX_ASSERT_CONT(this->Array == NULL);
      }
  }

  void Allocate(dax::Id numberOfValues)
  {
    if (this->NumberOfValues == numberOfValues) return;

    this->ReleaseResources();
    try
      {
      if (numberOfValues > 0)
        {
        AllocatorType allocator;
        this->Array = allocator.allocate(numberOfValues);
        this->NumberOfValues = numberOfValues;
        }
      else
        {
        // ReleaseResources should have already set NumberOfValues to 0.
        DAX_ASSERT_CONT(this->NumberOfValues == 0);
        }
      }
    catch (std::bad_alloc err)
      {
      // Make sureour state is OK.
      this->Array = NULL;
      this->NumberOfValues = 0;
      throw dax::cont::ErrorControlOutOfMemory(
            "Could not allocate basic control array.");
      }
  }

  dax::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(dax::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
      {
      throw dax::cont::ErrorControlBadValue(
            "Shrink method cannot be used to grow array.");
      }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal()
  {
    return PortalType(this->Array, this->NumberOfValues);
  }

  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array, this->NumberOfValues);
  }

private:
  // Not implemented.
  ArrayContainerControl(const ArrayContainerControl<ValueType, vtkToDax::vtkPointsContainerTag> &src);
  void operator=(const ArrayContainerControl<ValueType, vtkToDax::vtkPointsContainerTag> &src);

  PointerType Array;
  dax::Id NumberOfValues;
};

template <typename ValueT, typename CellType>
class ArrayContainerControl<ValueT,vtkToDax::vtkTopologyContainerTag<CellType> >
{
public:
  typedef ValueT ValueType;
  typedef vtkToDax::vtkTopologyPortal<ValueType, CellType::NUM_POINTS> PortalType;
  typedef vtkToDax::vtkTopologyPortal<const ValueType, CellType::NUM_POINTS > PortalConstType;

private:
  //determine the allocator type and pointer type for this container
  typedef vtkToDax::vtkAlloc<vtkCellArray, CellType::NUM_POINTS > AllocatorType;
  //the pointer type tells us the type of what the allocator returns
  typedef typename AllocatorType::pointer PointerType;

public:

  ArrayContainerControl() : Array(NULL), NumberOfValues(0) { }

  ~ArrayContainerControl()
  {
    this->ReleaseResources();
  }

  void ReleaseResources()
  {
    if (this->NumberOfValues > 0)
      {
      DAX_ASSERT_CONT(this->Array != NULL);
      AllocatorType allocator;
      allocator.deallocate(this->Array, this->NumberOfValues);
      this->Array = NULL;
      this->NumberOfValues = 0;
      }
    else
      {
      DAX_ASSERT_CONT(this->Array == NULL);
      }
  }

  void Allocate(dax::Id numberOfValues)
  {
    if (this->NumberOfValues == numberOfValues) return;

    this->ReleaseResources();
    try
      {
      if (numberOfValues > 0)
        {
        AllocatorType allocator;
        this->Array = allocator.allocate(numberOfValues);
        this->NumberOfValues = numberOfValues;
        }
      else
        {
        // ReleaseResources should have already set NumberOfValues to 0.
        DAX_ASSERT_CONT(this->NumberOfValues == 0);
        }
      }
    catch (std::bad_alloc err)
      {
      // Make sureour state is OK.
      this->Array = NULL;
      this->NumberOfValues = 0;
      throw dax::cont::ErrorControlOutOfMemory(
            "Could not allocate basic control array.");
      }
  }

  dax::Id GetNumberOfValues() const
  {
    return this->NumberOfValues;
  }

  void Shrink(dax::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
      {
      throw dax::cont::ErrorControlBadValue(
            "Shrink method cannot be used to grow array.");
      }

    this->NumberOfValues = numberOfValues;
  }

  PortalType GetPortal()
  {
    return PortalType(this->Array, this->NumberOfValues);
  }

  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array, this->NumberOfValues);
  }

private:
  // Not implemented.
  ArrayContainerControl(const ArrayContainerControl<ValueType, vtkToDax::vtkPointsContainerTag> &src);
  void operator=(const ArrayContainerControl<ValueType, vtkToDax::vtkPointsContainerTag> &src);

  PointerType Array;
  dax::Id NumberOfValues;
};

}
}
}

#endif //vtkToDax_CONTAINERS_H
