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
#ifndef viskores_exec_internal_FastVec_h
#define viskores_exec_internal_FastVec_h

#include <viskores/Types.h>
#include <viskores/VecVariable.h>

namespace viskores
{
namespace exec
{
namespace internal
{

/// Use this class to convert Vecs of any type to an efficient stack based Vec
/// type. The template parameters are the input Vec type and the maximum
/// number of components it may have. Specializations exist to optimize
/// the copy and stack usage away for already efficient types.
/// This class is useful when several accesses will be performed on
/// potentially inefficient Vec types such as VecFromPortalPermute.
///
template <typename VecType, viskores::IdComponent MaxSize>
class FastVec
{
public:
  using Type = viskores::VecVariable<typename VecType::ComponentType, MaxSize>;

  explicit VISKORES_EXEC FastVec(const VecType& vec)
    : Vec(vec)
  {
  }

  VISKORES_EXEC const Type& Get() const { return this->Vec; }

private:
  Type Vec;
};

template <typename ComponentType,
          viskores::IdComponent NumComponents,
          viskores::IdComponent MaxSize>
class FastVec<viskores::Vec<ComponentType, NumComponents>, MaxSize>
{
public:
  using Type = viskores::Vec<ComponentType, NumComponents>;

  explicit VISKORES_EXEC FastVec(const Type& vec)
    : Vec(vec)
  {
    VISKORES_ASSERT(vec.GetNumberOfComponents() <= MaxSize);
  }

  VISKORES_EXEC const Type& Get() const { return this->Vec; }

private:
  const Type& Vec;
};

template <typename ComponentType, viskores::IdComponent MaxSize1, viskores::IdComponent MaxSize2>
class FastVec<viskores::VecVariable<ComponentType, MaxSize1>, MaxSize2>
{
public:
  using Type = viskores::VecVariable<ComponentType, MaxSize1>;

  explicit VISKORES_EXEC FastVec(const Type& vec)
    : Vec(vec)
  {
    VISKORES_ASSERT(vec.GetNumberOfComponents() <= MaxSize2);
  }

  VISKORES_EXEC const Type& Get() const { return this->Vec; }

private:
  const Type& Vec;
};
}
}
} // viskores::exec::internal

#endif // viskores_exec_internal_FastVec_h
