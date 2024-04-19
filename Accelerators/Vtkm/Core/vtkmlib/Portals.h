// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_Portals_h
#define vtkmlib_Portals_h

#include "PortalTraits.h"
#include "vtkAcceleratorsVTKmCoreModule.h"
#include "vtkmConfigCore.h" //required for general vtkm setup

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkPoints;
VTK_ABI_NAMESPACE_END

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

template <typename Type, typename VTKDataArrayType_>
class VTKM_ALWAYS_EXPORT vtkArrayPortal
{
  static const int NUM_COMPONENTS = vtkm::VecTraits<Type>::NUM_COMPONENTS;

public:
  typedef VTKDataArrayType_ VTKDataArrayType;
  using ValueType = typename vtkPortalTraits<Type>::Type;
  using ComponentType = typename vtkPortalTraits<Type>::ComponentType;

  VTKM_EXEC_CONT
  vtkArrayPortal();

  VTKM_CONT
  vtkArrayPortal(VTKDataArrayType* array, vtkm::Id size);

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Size; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline ValueType Get(vtkm::Id index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void Set(vtkm::Id index, const ValueType& value) const;

  typedef vtkm::cont::internal::IteratorFromArrayPortal<vtkArrayPortal> IteratorType;

  VTKM_CONT
  IteratorType GetIteratorBegin() const { return IteratorType(*this, 0); }

  VTKM_CONT
  IteratorType GetIteratorEnd() const { return IteratorType(*this, this->Size); }

  VTKM_CONT
  VTKDataArrayType* GetVtkData() const { return this->VTKData; }

private:
  VTKDataArrayType* VTKData;
  vtkm::Id Size;
};

template <typename Type>
class VTKM_ALWAYS_EXPORT vtkPointsPortal
{
  static const int NUM_COMPONENTS = vtkm::VecTraits<Type>::NUM_COMPONENTS;

public:
  using ValueType = typename vtkPortalTraits<Type>::Type;
  using ComponentType = typename vtkPortalTraits<Type>::ComponentType;

  VTKM_EXEC_CONT
  vtkPointsPortal();

  VTKM_CONT
  vtkPointsPortal(vtkPoints* points, vtkm::Id size);

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Size; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline ValueType Get(vtkm::Id index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  inline void Set(vtkm::Id index, const ValueType& value) const;

  typedef vtkm::cont::internal::IteratorFromArrayPortal<vtkPointsPortal> IteratorType;

  VTKM_CONT
  IteratorType GetIteratorBegin() const { return IteratorType(*this, 0); }

  VTKM_CONT
  IteratorType GetIteratorEnd() const { return IteratorType(*this, this->Size); }

  VTKM_CONT
  vtkPoints* GetVtkData() const { return Points; }

private:
  vtkPoints* Points;
  ComponentType* Array;
  vtkm::Id Size;
};
VTK_ABI_NAMESPACE_END
}

#ifndef vtkmlib_Portals_cxx
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>
namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
// T extern template instantiations
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3> const>;
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3> const>;
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3>>;
extern template class VTKACCELERATORSVTKMCORE_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3>>;
VTK_ABI_NAMESPACE_END
}

#endif // defined vtkmlib_Portals_cxx

#include "Portals.hxx"
#endif // vtkmlib_Portals_h
/* VTK-HeaderTest-Exclude: Portals.h */
