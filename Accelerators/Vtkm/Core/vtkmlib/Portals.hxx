// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_Portals_hxx
#define vtkmlib_Portals_hxx

#include "vtkDataArray.h"
#include "vtkPoints.h"

#include <viskores/Types.h>
#include <viskores/VecTraits.h>
#include <viskores/cont/internal/IteratorFromArrayPortal.h>
#include <viskores/internal/Assume.h>

namespace
{

template <int N>
struct fillComponents
{
  template <typename T, typename Tuple>
  VISKORES_EXEC void operator()(T* t, const Tuple& tuple) const
  {
    fillComponents<N - 1>()(t, tuple);
    t[N - 1] = viskores::VecTraits<Tuple>::GetComponent(tuple, N - 1);
  }
};

template <>
struct fillComponents<1>
{
  template <typename T, typename Tuple>
  VISKORES_EXEC void operator()(T* t, const Tuple& tuple) const
  {
    t[0] = viskores::VecTraits<Tuple>::GetComponent(tuple, 0);
  }
};
}

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
VISKORES_EXEC_CONT vtkArrayPortal<VType, VTKDataArrayType>::vtkArrayPortal()
  : VTKData(nullptr)
  , Size(0)
{
}

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
VISKORES_CONT vtkArrayPortal<VType, VTKDataArrayType>::vtkArrayPortal(
  VTKDataArrayType* array, viskores::Id size)
  : VTKData(array)
  , Size(size)
{
  VISKORES_ASSERT(this->GetNumberOfValues() >= 0);
}

//------------------------------------------------------------------------------
template <typename VType, typename VTKDataArrayType>
typename vtkArrayPortal<VType, VTKDataArrayType>::ValueType VISKORES_EXEC
vtkArrayPortal<VType, VTKDataArrayType>::Get(viskores::Id index) const
{
  VISKORES_ASSUME(this->VTKData->GetNumberOfComponents() == NUM_COMPONENTS);

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
VISKORES_EXEC void vtkArrayPortal<VType, VTKDataArrayType>::Set(
  viskores::Id index, const ValueType& value) const
{
  VISKORES_ASSUME((this->VTKData->GetNumberOfComponents() == NUM_COMPONENTS));

  for (int j = 0; j < NUM_COMPONENTS; ++j)
  {
    ComponentType temp = vtkPortalTraits<ValueType>::GetComponent(value, j);
    this->VTKData->SetTypedComponent(index, j, temp);
  }
}

//------------------------------------------------------------------------------
template <typename Type>
VISKORES_EXEC_CONT vtkPointsPortal<Type>::vtkPointsPortal()
  : Points(nullptr)
  , Array(nullptr)
  , Size(0)
{
}

//------------------------------------------------------------------------------
template <typename Type>
VISKORES_CONT vtkPointsPortal<Type>::vtkPointsPortal(vtkPoints* points, viskores::Id size)
  : Points(points)
  , Array(static_cast<ComponentType*>(points->GetVoidPointer(0)))
  , Size(size)
{
  VISKORES_ASSERT(this->GetNumberOfValues() >= 0);
}

//------------------------------------------------------------------------------
template <typename Type>
typename vtkPointsPortal<Type>::ValueType VISKORES_EXEC vtkPointsPortal<Type>::Get(
  viskores::Id index) const
{
  const ComponentType* const raw = this->Array + (index * NUM_COMPONENTS);
  return ValueType(raw[0], raw[1], raw[2]);
}

//------------------------------------------------------------------------------
template <typename Type>
VISKORES_EXEC void vtkPointsPortal<Type>::Set(viskores::Id index, const ValueType& value) const
{
  ComponentType* rawArray = this->Array + (index * NUM_COMPONENTS);
  // use template magic to auto unroll insertion
  fillComponents<NUM_COMPONENTS>()(rawArray, value);
}
VTK_ABI_NAMESPACE_END
}

#endif // vtkmlib_Portals_hxx
