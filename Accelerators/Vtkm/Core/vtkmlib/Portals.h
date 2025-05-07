// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_Portals_h
#define vtkmlib_Portals_h

#include "PortalTraits.h"
#include "vtkAcceleratorsVTKmCoreModule.h"
#include "vtkmConfigCore.h" //required for general viskores setup

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkPoints;
VTK_ABI_NAMESPACE_END

#include <viskores/cont/internal/IteratorFromArrayPortal.h>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

template <typename Type, typename VTKDataArrayType_>
class VISKORES_ALWAYS_EXPORT vtkArrayPortal
{
  static const int NUM_COMPONENTS = viskores::VecTraits<Type>::NUM_COMPONENTS;

public:
  typedef VTKDataArrayType_ VTKDataArrayType;
  using ValueType = typename vtkPortalTraits<Type>::Type;
  using ComponentType = typename vtkPortalTraits<Type>::ComponentType;

  VISKORES_EXEC_CONT
  vtkArrayPortal();

  VISKORES_CONT
  vtkArrayPortal(VTKDataArrayType* array, viskores::Id size);

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->Size; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  inline ValueType Get(viskores::Id index) const;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  inline void Set(viskores::Id index, const ValueType& value) const;

  typedef viskores::cont::internal::IteratorFromArrayPortal<vtkArrayPortal> IteratorType;

  VISKORES_CONT
  IteratorType GetIteratorBegin() const { return IteratorType(*this, 0); }

  VISKORES_CONT
  IteratorType GetIteratorEnd() const { return IteratorType(*this, this->Size); }

  VISKORES_CONT
  VTKDataArrayType* GetVtkData() const { return this->VTKData; }

private:
  VTKDataArrayType* VTKData;
  viskores::Id Size;
};

template <typename Type>
class VISKORES_ALWAYS_EXPORT vtkPointsPortal
{
  static const int NUM_COMPONENTS = viskores::VecTraits<Type>::NUM_COMPONENTS;

public:
  using ValueType = typename vtkPortalTraits<Type>::Type;
  using ComponentType = typename vtkPortalTraits<Type>::ComponentType;

  VISKORES_EXEC_CONT
  vtkPointsPortal();

  VISKORES_CONT
  vtkPointsPortal(vtkPoints* points, viskores::Id size);

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->Size; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  inline ValueType Get(viskores::Id index) const;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  inline void Set(viskores::Id index, const ValueType& value) const;

  typedef viskores::cont::internal::IteratorFromArrayPortal<vtkPointsPortal> IteratorType;

  VISKORES_CONT
  IteratorType GetIteratorBegin() const { return IteratorType(*this, 0); }

  VISKORES_CONT
  IteratorType GetIteratorEnd() const { return IteratorType(*this, this->Size); }

  VISKORES_CONT
  vtkPoints* GetVtkData() const { return Points; }

private:
  vtkPoints* Points;
  ComponentType* Array;
  viskores::Id Size;
};
VTK_ABI_NAMESPACE_END
}

#ifndef vtkmlib_Portals_cxx
#include <viskores/cont/internal/ArrayPortalFromIterators.h>
namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
// T extern template instantiations
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<viskores::Vec<viskores::Float32, 3> const>;
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<viskores::Vec<viskores::Float64, 3> const>;
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<viskores::Vec<viskores::Float32, 3>>;
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<viskores::Vec<viskores::Float64, 3>>;
VTK_ABI_NAMESPACE_END
}

#endif // defined vtkmlib_Portals_cxx

#include "Portals.hxx"
#endif // vtkmlib_Portals_h
/* VTK-HeaderTest-Exclude: Portals.h */
