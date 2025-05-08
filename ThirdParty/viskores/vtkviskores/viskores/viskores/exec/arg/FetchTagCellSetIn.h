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
#ifndef viskores_exec_arg_FetchTagCellSetIn_h
#define viskores_exec_arg_FetchTagCellSetIn_h

#include <viskores/exec/arg/AspectTagDefault.h>
#include <viskores/exec/arg/Fetch.h>

namespace viskores
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting topology information.
///
/// \c FetchTagCellSetIn is a tag used with the \c Fetch class to retrieve
/// values from a topology object.  This default parameter returns
/// the basis topology type, i.e. cell type in a \c WorkletCellMap.
///
struct FetchTagCellSetIn
{
};

template <typename ExecObjectType>
struct Fetch<viskores::exec::arg::FetchTagCellSetIn,
             viskores::exec::arg::AspectTagDefault,
             ExecObjectType>
{
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VISKORES_EXEC auto Load(const ThreadIndicesType& indices, const ExecObjectType&) const
    -> decltype(indices.GetCellShape())
  {
    return indices.GetCellShape();
  }

  template <typename ThreadIndicesType, typename ValueType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace viskores::exec::arg

#endif //viskores_exec_arg_FetchTagCellSetIn_h
