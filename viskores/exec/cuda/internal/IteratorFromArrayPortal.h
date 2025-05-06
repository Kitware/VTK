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
#ifndef viskores_exec_cuda_internal_IteratorFromArrayPortal_h
#define viskores_exec_cuda_internal_IteratorFromArrayPortal_h

#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/internal/ArrayPortalValueReference.h>
#include <viskores/internal/ExportMacros.h>

// Disable warnings we check viskores for but Thrust does not.
#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <thrust/functional.h>
#include <thrust/iterator/iterator_facade.h>
#include <thrust/system/cuda/execution_policy.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace exec
{
namespace cuda
{
namespace internal
{

template <class ArrayPortalType>
class IteratorFromArrayPortal
  : public ::thrust::iterator_facade<IteratorFromArrayPortal<ArrayPortalType>,
                                     typename ArrayPortalType::ValueType,
                                     ::thrust::system::cuda::tag,
                                     ::thrust::random_access_traversal_tag,
                                     viskores::internal::ArrayPortalValueReference<ArrayPortalType>,
                                     std::ptrdiff_t>
{
public:
  VISKORES_EXEC_CONT
  IteratorFromArrayPortal()
    : Portal()
    , Index(0)
  {
  }

  VISKORES_CONT
  explicit IteratorFromArrayPortal(const ArrayPortalType& portal, viskores::Id index = 0)
    : Portal(portal)
    , Index(index)
  {
  }

  VISKORES_EXEC
  viskores::internal::ArrayPortalValueReference<ArrayPortalType> operator[](
    std::ptrdiff_t idx) const //NEEDS to be signed
  {
    return viskores::internal::ArrayPortalValueReference<ArrayPortalType>(
      this->Portal, this->Index + static_cast<viskores::Id>(idx));
  }

private:
  ArrayPortalType Portal;
  viskores::Id Index;

  // Implementation for ::thrust iterator_facade
  friend class ::thrust::iterator_core_access;

  VISKORES_EXEC
  viskores::internal::ArrayPortalValueReference<ArrayPortalType> dereference() const
  {
    return viskores::internal::ArrayPortalValueReference<ArrayPortalType>(this->Portal,
                                                                          this->Index);
  }

  VISKORES_EXEC
  bool equal(const IteratorFromArrayPortal<ArrayPortalType>& other) const
  {
    // Technically, we should probably check that the portals are the same,
    // but the portal interface does not specify an equal operator.  It is
    // by its nature undefined what happens when comparing iterators from
    // different portals anyway.
    return (this->Index == other.Index);
  }

  VISKORES_EXEC_CONT
  void increment() { this->Index++; }

  VISKORES_EXEC_CONT
  void decrement() { this->Index--; }

  VISKORES_EXEC_CONT
  void advance(std::ptrdiff_t delta) { this->Index += static_cast<viskores::Id>(delta); }

  VISKORES_EXEC_CONT
  std::ptrdiff_t distance_to(const IteratorFromArrayPortal<ArrayPortalType>& other) const
  {
    // Technically, we should probably check that the portals are the same,
    // but the portal interface does not specify an equal operator.  It is
    // by its nature undefined what happens when comparing iterators from
    // different portals anyway.
    return static_cast<std::ptrdiff_t>(other.Index - this->Index);
  }
};
}
}
}
} //namespace viskores::exec::cuda::internal

//So for the unary_transform_functor and binary_transform_functor inside
//of thrust, they verify that the index they are storing into is a reference
//instead of a value, so that the contents actually are written to global memory.
//
//But for viskores we pass in facade objects, which are passed by value, but
//must be treated as references. So do to do that properly we need to specialize
//is_non_const_reference to state an ArrayPortalValueReference by value is valid
//for writing
namespace thrust
{
namespace detail
{

template <typename T>
struct is_non_const_reference;

template <typename T>
struct is_non_const_reference<viskores::internal::ArrayPortalValueReference<T>>
  : thrust::detail::true_type
{
};
}
}

#endif //viskores_exec_cuda_internal_IteratorFromArrayPortal_h
