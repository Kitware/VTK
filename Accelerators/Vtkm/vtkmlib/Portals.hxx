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

#ifndef vtkmlib_Portals_hxx
#define vtkmlib_Portals_hxx

#include "vtkDataArray.h"
#include "vtkPoints.h"

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>
#include <vtkm/cont/internal/IteratorFromArrayPortal.h>
#include <vtkm/internal/Assume.h>

namespace {

template <int N> struct fillComponents
{
  template <typename T, typename Tuple>
  void operator()(T* t, const Tuple& tuple) const
  {
    fillComponents<N - 1>()(t, tuple);
    t[N - 1] = vtkm::VecTraits<Tuple>::GetComponent(tuple, N - 1);
  }
};

template <> struct fillComponents<1>
{
  template <typename T, typename Tuple>
  void operator()(T* t, const Tuple& tuple) const
  {
    t[0] = vtkm::VecTraits<Tuple>::GetComponent(tuple, 0);
  }
};
}

namespace tovtkm {

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
vtkArrayPortal<VType, VTKDataArrayType>::vtkArrayPortal()
  : VTKData(NULL), Size(0)
{
}

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
vtkArrayPortal<VType, VTKDataArrayType>::vtkArrayPortal(VTKDataArrayType* array,
                                                        vtkm::Id size)
  : VTKData(array), Size(size)
{
  VTKM_ASSERT(this->GetNumberOfValues() >= 0);
}

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
typename vtkArrayPortal<VType, VTKDataArrayType>::ValueType
vtkArrayPortal<VType, VTKDataArrayType>::Get(vtkm::Id index) const
{
  VTKM_ASSUME(this->VTKData->GetNumberOfComponents() == NUM_COMPONENTS);

  ValueType tuple;
  for (int j = 0; j < NUM_COMPONENTS; ++j)
  {
    ComponentType temp = this->VTKData->GetTypedComponent(index, j);
    vtkPortalTraits<ValueType>::SetComponent(tuple, j, temp);
  }
  return tuple;
}

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
void vtkArrayPortal<VType, VTKDataArrayType>::Set(vtkm::Id index,
                                                  const ValueType& value) const
{
  VTKM_ASSUME((this->VTKData->GetNumberOfComponents() == NUM_COMPONENTS));

  for (int j = 0; j < NUM_COMPONENTS; ++j)
  {
    ComponentType temp = vtkPortalTraits<ValueType>::GetComponent(value, j);
    this->VTKData->SetTypedComponent(index, j, temp);
  }
}

//------------------------------------------------------------------------------
template <typename Type>
vtkPointsPortal<Type>::vtkPointsPortal() : Points(NULL), Array(NULL), Size(0)
{
}

//------------------------------------------------------------------------------
template <typename Type>
vtkPointsPortal<Type>::vtkPointsPortal(vtkPoints* points, vtkm::Id size)
  : Points(points),
    Array(static_cast<ComponentType*>(points->GetVoidPointer(0))),
    Size(size)
{
  VTKM_ASSERT(this->GetNumberOfValues() >= 0);
}

//------------------------------------------------------------------------------
/// Copy constructor for any other vtkArrayPortal with an iterator
/// type that can be copied to this iterator type. This allows us to do any
/// type casting that the iterators do (like the non-const to const cast).
///
template <typename Type>
template <typename OtherType>
vtkPointsPortal<Type>::vtkPointsPortal(const vtkPointsPortal<OtherType>& src)
  : Points(src.GetVtkData()),
    Array(static_cast<ComponentType*>(src.GetVtkData()->GetVoidPointer(0))),
    Size(src.GetNumberOfValues())
{
}

//------------------------------------------------------------------------------
template <typename Type>
typename vtkPointsPortal<Type>::ValueType
vtkPointsPortal<Type>::Get(vtkm::Id index) const
{
  const ComponentType* const raw = this->Array + (index * NUM_COMPONENTS);
  return ValueType(raw[0], raw[1], raw[2]);
}

//------------------------------------------------------------------------------
template <typename Type>
void vtkPointsPortal<Type>::Set(vtkm::Id index, const ValueType& value) const
{
  ComponentType* rawArray = this->Array + (index * NUM_COMPONENTS);
  // use template magic to auto unroll insertion
  fillComponents<NUM_COMPONENTS>()(rawArray, value);
}
}

#endif // vtkmlib_Portals_hxx
