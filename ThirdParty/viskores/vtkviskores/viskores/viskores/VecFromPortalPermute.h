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
#ifndef viskores_VecFromPortalPermute_h
#define viskores_VecFromPortalPermute_h

#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

/// \brief A short vector from an ArrayPortal and a vector of indices.
///
/// The \c VecFromPortalPermute class is a Vec-like class that holds an array
/// portal and a second Vec-like containing indices into the array. Each value
/// of this vector is the value from the array with the respective index.
///
template <typename IndexVecType, typename PortalType>
class VecFromPortalPermute
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  VecFromPortalPermute() {}

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  VecFromPortalPermute(const IndexVecType* indices, const PortalType& portal)
    : Indices(indices)
    , Portal(portal)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfComponents() const
  {
    return this->Indices->GetNumberOfComponents();
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT void CopyInto(viskores::Vec<ComponentType, DestSize>& dest) const
  {
    viskores::IdComponent numComponents = viskores::Min(DestSize, this->GetNumberOfComponents());
    for (viskores::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ComponentType operator[](viskores::IdComponent index) const
  {
    return this->Portal.Get((*this->Indices)[index]);
  }

private:
  const IndexVecType* const Indices;
  PortalType Portal;
};

template <typename IndexVecType, typename PortalType>
class VecFromPortalPermute<IndexVecType, const PortalType*>
{
public:
  using ComponentType = typename std::remove_const<typename PortalType::ValueType>::type;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  VecFromPortalPermute() {}

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  VecFromPortalPermute(const IndexVecType* indices, const PortalType* const portal)
    : Indices(indices)
    , Portal(portal)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::IdComponent GetNumberOfComponents() const
  {
    return this->Indices->GetNumberOfComponents();
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT void CopyInto(viskores::Vec<ComponentType, DestSize>& dest) const
  {
    viskores::IdComponent numComponents = viskores::Min(DestSize, this->GetNumberOfComponents());
    for (viskores::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = (*this)[index];
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ComponentType operator[](viskores::IdComponent index) const
  {
    return this->Portal->Get((*this->Indices)[index]);
  }

private:
  const IndexVecType* const Indices;
  const PortalType* const Portal;
};

template <typename IndexVecType, typename PortalType>
struct TypeTraits<viskores::VecFromPortalPermute<IndexVecType, PortalType>>
{
private:
  using VecType = viskores::VecFromPortalPermute<IndexVecType, PortalType>;
  using ComponentType = typename PortalType::ValueType;

public:
  using NumericTag = typename viskores::TypeTraits<ComponentType>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  static VecType ZeroInitialization() { return VecType(); }
};

template <typename IndexVecType, typename PortalType>
struct VecTraits<viskores::VecFromPortalPermute<IndexVecType, PortalType>>
{
  using VecType = viskores::VecFromPortalPermute<IndexVecType, PortalType>;

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
  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename IndexVecType, typename PortalType>
inline VISKORES_EXEC VecFromPortalPermute<IndexVecType, PortalType> make_VecFromPortalPermute(
  const IndexVecType* index,
  const PortalType& portal)
{
  return VecFromPortalPermute<IndexVecType, PortalType>(index, portal);
}

template <typename IndexVecType, typename PortalType>
inline VISKORES_EXEC VecFromPortalPermute<IndexVecType, const PortalType*>
make_VecFromPortalPermute(const IndexVecType* index, const PortalType* const portal)
{
  return VecFromPortalPermute<IndexVecType, const PortalType*>(index, portal);
}

} // namespace viskores

#endif //viskores_VecFromPortalPermute_h
