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

#ifndef vtkmlib_Portals_h
#define vtkmlib_Portals_h

#include "PortalTraits.h"
#include "vtkAcceleratorsVTKmModule.h"
#include "vtkmConfig.h" //required for general vtkm setup

class vtkDataArray;
class vtkPoints;

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

namespace tovtkm
{

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
}

#ifndef vtkmlib_Portals_cxx
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>
namespace tovtkm
{
// T extern template instantiations
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3> const>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3> const>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3> >;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
  vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3> >;
}

#endif // defined vtkmlib_Portals_cxx

#include "Portals.hxx"
#endif // vtkmlib_Portals_h
