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

#include "vtkAcceleratorsVTKmModule.h"
#include "vtkmConfig.h" //required for general vtkm setup
#include "PortalTraits.h"
#include "vtkmTags.h"

class vtkDataArray;
class vtkPoints;

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

namespace tovtkm {

template <typename Type, typename VTKDataArrayType_>
class VTKM_ALWAYS_EXPORT vtkArrayPortal
{
  static const int NUM_COMPONENTS = vtkm::VecTraits<Type>::NUM_COMPONENTS;

public:
  typedef VTKDataArrayType_ VTKDataArrayType;
  using ValueType = typename vtkPortalTraits<Type>::Type;
  using ComponentType = typename vtkPortalTraits<Type>::ComponentType;

  vtkArrayPortal();

  vtkArrayPortal(VTKDataArrayType* array, vtkm::Id size);

  vtkm::Id GetNumberOfValues() const
  {
    return this->Size;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Get(vtkm::Id index) const;

  void Set(vtkm::Id index, const ValueType& value) const;

  typedef vtkm::cont::internal::IteratorFromArrayPortal<vtkArrayPortal>
      IteratorType;

  IteratorType GetIteratorBegin() const
  {
    return IteratorType(*this, 0);
  }

  IteratorType GetIteratorEnd() const
  {
    return IteratorType(*this, this->Size);
  }

  VTKDataArrayType* GetVtkData() const
  {
    return this->VTKData;
  }

private:
  VTKDataArrayType* VTKData;
  vtkm::Id Size;
};

template <typename Type> class VTKM_ALWAYS_EXPORT vtkPointsPortal
{
  static const int NUM_COMPONENTS = vtkm::VecTraits<Type>::NUM_COMPONENTS;

public:
  using ValueType = typename vtkPortalTraits<Type>::Type;
  using ComponentType = typename vtkPortalTraits<Type>::ComponentType;

  vtkPointsPortal();

  vtkPointsPortal(vtkPoints* points, vtkm::Id size);

  /// Copy constructor for any other vtkArrayPortal with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <typename OtherType>
  vtkPointsPortal(const vtkPointsPortal<OtherType>& src);

  vtkm::Id GetNumberOfValues() const
  {
    return this->Size;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Get(vtkm::Id index) const;

  void Set(vtkm::Id index, const ValueType& value) const;

  typedef vtkm::cont::internal::IteratorFromArrayPortal<vtkPointsPortal>
      IteratorType;

  IteratorType GetIteratorBegin() const
  {
    return IteratorType(*this, 0);
  }

  IteratorType GetIteratorEnd() const
  {
    return IteratorType(*this, this->Size);
  }

  vtkPoints* GetVtkData() const
  {
    return Points;
  }

private:
  vtkPoints* Points;
  ComponentType* Array;
  vtkm::Id Size;
};
}

#define VTKM_TEMPLATE_EXPORT_ArrayPortal(T, S)                                 \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      vtkArrayPortal<const T, S<T>>;                                           \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT vtkArrayPortal<T, S<T>>;    \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                             \
      vtkArrayPortal<const vtkm::Vec<T, 2>, S<T>>;                             \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      vtkArrayPortal<vtkm::Vec<T, 2>, S<T>>;                                   \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      vtkArrayPortal<const vtkm::Vec<T, 3>, S<T>>;                             \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      vtkArrayPortal<vtkm::Vec<T, 3>, S<T>>;                                   \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      vtkArrayPortal<const vtkm::Vec<T, 4>, S<T>>;                             \
  extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                    \
      vtkArrayPortal<vtkm::Vec<T, 4>, S<T>>;

#define VTKM_TEMPLATE_IMPORT_ArrayPortal(T, S)                                 \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT vtkArrayPortal<const T, S<T>>;     \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT vtkArrayPortal<T, S<T>>;  \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                           \
      vtkArrayPortal<const vtkm::Vec<T, 2>, S<T>>;                             \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                           \
      vtkArrayPortal<vtkm::Vec<T, 2>, S<T>>;                                   \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                           \
      vtkArrayPortal<const vtkm::Vec<T, 3>, S<T>>;                             \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                           \
      vtkArrayPortal<vtkm::Vec<T, 3>, S<T>>;                                   \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                           \
      vtkArrayPortal<const vtkm::Vec<T, 4>, S<T>>;                             \
  template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT                           \
      vtkArrayPortal<vtkm::Vec<T, 4>, S<T>>;

#ifndef vtkmlib_Portals_cxx
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>
namespace tovtkm {
// T extern template instantiations
VTKM_TEMPLATE_EXPORT_ArrayPortal(char, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int8, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt8, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int16, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt16, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int32, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt32, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int64, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt64, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Float32, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Float64, vtkAOSDataArrayTemplate);

VTKM_TEMPLATE_EXPORT_ArrayPortal(char, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int8, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt8, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int16, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt16, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int32, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt32, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Int64, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::UInt64, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Float32, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(vtkm::Float64, vtkSOADataArrayTemplate);

#if VTKM_SIZE_LONG_LONG == 8
VTKM_TEMPLATE_EXPORT_ArrayPortal(long, vtkAOSDataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(unsigned long, vtkAOSDataArrayTemplate);

VTKM_TEMPLATE_EXPORT_ArrayPortal(long, vtkSOADataArrayTemplate);
VTKM_TEMPLATE_EXPORT_ArrayPortal(unsigned long, vtkSOADataArrayTemplate);
#endif

extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3> const>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3> const>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float32, 3>>;
extern template class VTKACCELERATORSVTKM_TEMPLATE_EXPORT
    vtkPointsPortal<vtkm::Vec<vtkm::Float64, 3>>;
}

#endif // defined vtkmlib_Portals_cxx

#include "Portals.hxx"
#endif // vtkmlib_Portals_h
