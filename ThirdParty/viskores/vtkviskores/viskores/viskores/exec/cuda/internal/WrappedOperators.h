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
#ifndef viskores_exec_cuda_internal_WrappedOperators_h
#define viskores_exec_cuda_internal_WrappedOperators_h

#include <viskores/BinaryPredicates.h>
#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/exec/cuda/internal/IteratorFromArrayPortal.h>
#include <viskores/internal/ExportMacros.h>

// Disable warnings we check viskores for but Thrust does not.
#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <thrust/system/cuda/memory.h>
VISKORES_THIRDPARTY_POST_INCLUDE

#if THRUST_VERSION >= 200500
#include <cuda/std/type_traits>
#endif
#if THRUST_VERSION >= 300300
#include <cuda/functional>
#endif

namespace viskores
{
namespace exec
{
namespace cuda
{
namespace internal
{

// Unary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename T_, typename Function>
struct WrappedUnaryPredicate
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects unary operators to have
  using first_argument_type = T;
  using result_type = bool;

  Function m_f;

  VISKORES_EXEC
  WrappedUnaryPredicate()
    : m_f()
  {
  }

  VISKORES_CONT
  WrappedUnaryPredicate(const Function& f)
    : m_f(f)
  {
  }

  VISKORES_EXEC bool operator()(const T& x) const { return m_f(x); }

  template <typename U>
  VISKORES_EXEC bool operator()(const viskores::internal::ArrayPortalValueReference<U>& x) const
  {
    return m_f(x.Get());
  }

  VISKORES_EXEC bool operator()(const T* x) const { return m_f(*x); }
};

// Binary function object wrapper which can detect and handle calling the
// wrapped operator with complex value types such as
// ArrayPortalValueReference which happen when passed an input array that
// is implicit.
template <typename T_, typename Function>
struct WrappedBinaryOperator
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects binary operators to have
  using first_argument_type = T;
  using second_argument_type = T;
  using result_type = T;

  Function m_f;

  VISKORES_EXEC
  WrappedBinaryOperator()
    : m_f()
  {
  }

  VISKORES_CONT
  WrappedBinaryOperator(const Function& f)
    : m_f(f)
  {
  }

  VISKORES_EXEC T operator()(const T& x, const T& y) const { return m_f(x, y); }

  template <typename U>
  VISKORES_EXEC T operator()(const T& x,
                             const viskores::internal::ArrayPortalValueReference<U>& y) const
  {
    // to support proper implicit conversion, and avoid overload
    // ambiguities.
    return m_f(x, y.Get());
  }

  template <typename U>
  VISKORES_EXEC T operator()(const viskores::internal::ArrayPortalValueReference<U>& x,
                             const T& y) const
  {
    return m_f(x.Get(), y);
  }

  template <typename U, typename V>
  VISKORES_EXEC T operator()(const viskores::internal::ArrayPortalValueReference<U>& x,
                             const viskores::internal::ArrayPortalValueReference<V>& y) const
  {
    return m_f(x.Get(), y.Get());
  }

  VISKORES_EXEC T operator()(const T* const x, const T& y) const { return m_f(*x, y); }

  VISKORES_EXEC T operator()(const T& x, const T* const y) const { return m_f(x, *y); }

  VISKORES_EXEC T operator()(const T* const x, const T* const y) const { return m_f(*x, *y); }
};

template <typename T_, typename Function>
struct WrappedBinaryPredicate
{
  using T = typename std::remove_const<T_>::type;

  //make typedefs that thust expects binary operators to have
  using first_argument_type = T;
  using second_argument_type = T;
  using result_type = bool;

  Function m_f;

  VISKORES_EXEC
  WrappedBinaryPredicate()
    : m_f()
  {
  }

  VISKORES_CONT
  WrappedBinaryPredicate(const Function& f)
    : m_f(f)
  {
  }

  VISKORES_EXEC bool operator()(const T& x, const T& y) const { return m_f(x, y); }

  template <typename U>
  VISKORES_EXEC bool operator()(const T& x,
                                const viskores::internal::ArrayPortalValueReference<U>& y) const
  {
    return m_f(x, y.Get());
  }

  template <typename U>
  VISKORES_EXEC bool operator()(const viskores::internal::ArrayPortalValueReference<U>& x,
                                const T& y) const
  {
    return m_f(x.Get(), y);
  }

  template <typename U, typename V>
  VISKORES_EXEC bool operator()(const viskores::internal::ArrayPortalValueReference<U>& x,
                                const viskores::internal::ArrayPortalValueReference<V>& y) const
  {
    return m_f(x.Get(), y.Get());
  }

  VISKORES_EXEC bool operator()(const T* const x, const T& y) const { return m_f(*x, y); }

  VISKORES_EXEC bool operator()(const T& x, const T* const y) const { return m_f(x, *y); }

  VISKORES_EXEC bool operator()(const T* const x, const T* const y) const { return m_f(*x, *y); }
};
}
}
}
} //namespace viskores::exec::cuda::internal

//
// We tell Thrust/CUDA that our WrappedBinaryOperator is commutative so that we
// activate fast paths which are only available when the binary functor is
// commutative and the T type is is_arithmetic.
//
//
#if THRUST_VERSION >= 300300
namespace cuda
{
template <typename T, typename F>
inline constexpr bool
  is_commutative_v<::viskores::exec::cuda::internal::WrappedBinaryOperator<T, F>, T> =
    ::cuda::std::is_arithmetic<T>::value;
}
#else // THRUST_VERSION < 300300
VISKORES_THRUST_NAMESPACE_BEGIN
namespace detail
{
#if THRUST_VERSION >= 200500
template <typename T, typename F>
struct is_commutative<viskores::exec::cuda::internal::WrappedBinaryOperator<T, F>>
  : public ::cuda::std::is_arithmetic<T>
{
};
#else  // THRUST_VERSION < 200500
template <typename T, typename F>
struct is_commutative<viskores::exec::cuda::internal::WrappedBinaryOperator<T, F>>
  : public ::thrust::detail::is_arithmetic<T>
{
};
#endif // THRUST_VERSION < 200500
}
VISKORES_THRUST_NAMESPACE_END //namespace thrust::detail
#endif // THRUST_VERSION < 300300

#endif //viskores_exec_cuda_internal_WrappedOperators_h
