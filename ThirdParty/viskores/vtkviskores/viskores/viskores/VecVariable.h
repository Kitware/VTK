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
#ifndef viskores_VecVariable_h
#define viskores_VecVariable_h

#include <viskores/Assert.h>
#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

/// \brief A short variable-length array with maximum length.
///
/// The \c VecVariable class is a Vec-like class that holds a short array of
/// some maximum length. To avoid dynamic allocations, the maximum length is
/// specified at compile time. Internally, \c VecVariable holds a \c Vec of
/// the maximum length and exposes a subsection of it.
///
template <typename T, viskores::IdComponent MaxSize>
class VecVariable
{
public:
  using ComponentType = T;

  VISKORES_EXEC_CONT
  VecVariable()
    : NumComponents(0)
  {
  }

  template <typename SrcVecType>
  VISKORES_EXEC_CONT VecVariable(const SrcVecType& src)
    : NumComponents(src.GetNumberOfComponents())
  {
    VISKORES_ASSERT(this->NumComponents <= MaxSize);
    for (viskores::IdComponent index = 0; index < this->NumComponents; index++)
    {
      this->Data[index] = src[index];
    }
  }

  VISKORES_EXEC_CONT
  inline viskores::IdComponent GetNumberOfComponents() const { return this->NumComponents; }

  template <viskores::IdComponent DestSize>
  VISKORES_EXEC_CONT inline void CopyInto(viskores::Vec<ComponentType, DestSize>& dest) const
  {
    viskores::IdComponent numComponents = viskores::Min(DestSize, this->NumComponents);
    for (viskores::IdComponent index = 0; index < numComponents; index++)
    {
      dest[index] = this->Data[index];
    }
  }

  VISKORES_EXEC_CONT
  inline const ComponentType& operator[](viskores::IdComponent index) const
  {
    VISKORES_ASSERT(index >= 0 && index < this->NumComponents);
    return this->Data[index];
  }

  VISKORES_EXEC_CONT
  inline ComponentType& operator[](viskores::IdComponent index)
  {
    VISKORES_ASSERT(index >= 0 && index < this->NumComponents);
    return this->Data[index];
  }

  VISKORES_EXEC_CONT
  void Append(ComponentType value)
  {
    VISKORES_ASSERT(this->NumComponents < MaxSize);
    this->Data[this->NumComponents] = value;
    this->NumComponents++;
  }

private:
  viskores::Vec<T, MaxSize> Data;
  viskores::IdComponent NumComponents;
};

template <typename T, viskores::IdComponent MaxSize>
struct TypeTraits<viskores::VecVariable<T, MaxSize>>
{
  using NumericTag = typename viskores::TypeTraits<T>::NumericTag;
  using DimensionalityTag = TypeTraitsVectorTag;

  VISKORES_EXEC_CONT
  static viskores::VecVariable<T, MaxSize> ZeroInitialization()
  {
    return viskores::VecVariable<T, MaxSize>();
  }
};

template <typename T, viskores::IdComponent MaxSize>
struct VecTraits<viskores::VecVariable<T, MaxSize>>
{
  using VecType = viskores::VecVariable<T, MaxSize>;

  using ComponentType = typename VecType::ComponentType;
  using BaseComponentType = typename viskores::VecTraits<ComponentType>::BaseComponentType;
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeVariable;

  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector,
                                           viskores::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, viskores::IdComponent componentIndex)
  {
    return vector[componentIndex];
  }

  VISKORES_EXEC_CONT
  static void SetComponent(VecType& vector,
                           viskores::IdComponent componentIndex,
                           const ComponentType& value)
  {
    vector[componentIndex] = value;
  }

  template <typename NewComponentType>
  using ReplaceComponentType = viskores::VecVariable<NewComponentType, MaxSize>;

  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    viskores::VecVariable<typename viskores::VecTraits<
                            ComponentType>::template ReplaceBaseComponentType<NewComponentType>,
                          MaxSize>;

  template <viskores::IdComponent destSize>
  VISKORES_EXEC_CONT static void CopyInto(const VecType& src,
                                          viskores::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

} // namespace viskores

#endif //viskores_VecVariable_h
