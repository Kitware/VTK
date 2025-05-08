//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_VecFromPortal_h
#define viskores_VecFromPortal_h

#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

#include <viskores/internal/ArrayPortalValueReference.h>

namespace viskores
{

/// \brief A short variable-length array from a window in an ArrayPortal.
///
/// The \c VecFromPortal class is a Vec-like class that holds an array portal
/// and exposes a small window of that portal as if it were a \c Vec.
///
template <typename PortalType>
class VecFromPortal
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  VecFromPortal(const PortalType& portal,
                viskores::IdComponent numComponents = 0,
                viskores::Id offset = 0)
    : Portal(portal)
    , NumComponents(numComponents)
    , Offset(offset)
  {
  }

  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfComponents() const { return this->NumComponents; }

  template <typename T, viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT void CopyInto(viskores::Vec<T, DestSize>& dest) const
  {
    viskores::IdComponent numComponents = viskores::Min(DestSize, this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Portal.Get(index + this->Offset);
    }
  }

  template <viskores::IdComponent N>
  VISKORES_EXEC_CONT operator viskores::Vec<ComponentType, N>() const
  {
    viskores::Vec<ComponentType, N> result;
    this->CopyInto(result);
    for (viskores::IdComponent index = this->NumComponents; index < N; ++index)
    {
      result[index] = viskores::TypeTraits<ComponentType>::ZeroInitialization();
    }
    return result;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::internal::ArrayPortalValueReference<PortalType> operator[](
    viskores::IdComponent index) const
  {
    return viskores::internal::ArrayPortalValueReference<PortalType>(this->Portal,
                                                                     index + this->Offset);
  }

  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT VecFromPortal& operator=(const OtherVecType& src)
  {
    viskores::IdComponent numComponents =
      viskores::Min(src.GetNumberOfComponents(), this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; ++index)
    {
      this->Portal.Set(index + this->Offset, src[index]);
    }
    return *this;
  }

  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT VecFromPortal& operator+=(const OtherVecType& other)
  {
    viskores::IdComponent numComponents =
      viskores::Min(other.GetNumberOfComponents(), this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; ++index)
    {
      (*this)[index] += other[index];
    }
    return *this;
  }

  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT VecFromPortal& operator-=(const OtherVecType& other)
  {
    viskores::IdComponent numComponents =
      viskores::Min(other.GetNumberOfComponents(), this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; ++index)
    {
      (*this)[index] -= other[index];
    }
    return *this;
  }

private:
  template <typename OtherVecType>
  VISKORES_EXEC_CONT void Multiply(const OtherVecType& other, viskores::TypeTraitsVectorTag)
  {
    viskores::IdComponent numComponents =
      viskores::Min(other.GetNumberOfComponents(), this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; ++index)
    {
      (*this)[index] *= other[index];
    }
  }

  template <typename ScalarType>
  VISKORES_EXEC_CONT void Multiply(ScalarType other, viskores::TypeTraitsScalarTag)
  {
    for (viskores::IdComponent index = 0; index < this->NumComponents; ++index)
    {
      (*this)[index] *= other;
    }
  }

public:
  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT VecFromPortal& operator*=(const OtherVecType& other)
  {
    this->Multiply(other, typename viskores::TypeTraits<OtherVecType>::DimensionalityTag{});
    return *this;
  }

  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT VecFromPortal& operator/=(const OtherVecType& other)
  {
    viskores::IdComponent numComponents =
      viskores::Min(other.GetNumberOfComponents(), this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; ++index)
    {
      (*this)[index] /= other[index];
    }
    return *this;
  }

  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT bool operator==(const OtherVecType& other)
  {
    if (this->NumComponents != other.GetNumberOfComponents())
    {
      return false;
    }
    for (viskores::IdComponent index = 0; index < this->NumComponents; ++index)
    {
      if (this->Portal.Get(index + this->Offset) != other[index])
      {
        return false;
      }
    }
    return true;
  }

  // Only works with Vec-like objects with operator[] and GetNumberofComponents
  template <typename OtherVecType>
  VISKORES_EXEC_CONT bool operator!=(const OtherVecType& other)
  {
    return !(*this == other);
  }

  VISKORES_EXEC_CONT const PortalType& GetPortal() const { return this->Portal; }
  VISKORES_EXEC_CONT viskores::Id GetOffset() const { return this->Offset; }

private:
  PortalType Portal;
  viskores::IdComponent NumComponents;
  viskores::Id Offset;
};

template <typename PortalType>
struct TypeTraits<viskores::VecFromPortal<PortalType>>
{
private:
  using ComponentType = typename PortalType::ValueType;

public:
  using NumericTag = typename viskores::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  static viskores::VecFromPortal<PortalType> ZeroInitialization()
  {
    return viskores::VecFromPortal<PortalType>();
  }
};

template <typename PortalType>
struct VecTraits<viskores::VecFromPortal<PortalType>>
{
  using VecType = viskores::VecFromPortal<PortalType>;

  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename viskores::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeVariable;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  static ComponentType GetComponent(const VecType& vector, viskores::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  static void SetComponent(const VecType& vector,
                           viskores::IdComponent componentIndex,
                           const ComponentType& value)
  {
    vector[componentIndex] = value;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace viskores

#endif //viskores_VecFromPortal_h
