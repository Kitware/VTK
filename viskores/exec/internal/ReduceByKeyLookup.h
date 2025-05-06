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
#ifndef viskores_exec_internal_ReduceByKeyLookup_h
#define viskores_exec_internal_ReduceByKeyLookup_h

#include <viskores/cont/ExecutionObjectBase.h>

#include <viskores/StaticAssert.h>
#include <viskores/Types.h>

#include <type_traits>

namespace viskores
{
namespace exec
{
namespace internal
{

/// A superclass of `ReduceBykeyLookup` that can be used when no key values are provided.
///
template <typename IdPortalType, typename IdComponentPortalType>
struct ReduceByKeyLookupBase
{
  VISKORES_STATIC_ASSERT((std::is_same<typename IdPortalType::ValueType, viskores::Id>::value));
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename IdComponentPortalType::ValueType, viskores::IdComponent>::value));

  IdPortalType SortedValuesMap;
  IdPortalType Offsets;

  VISKORES_EXEC_CONT
  ReduceByKeyLookupBase(const IdPortalType& sortedValuesMap, const IdPortalType& offsets)
    : SortedValuesMap(sortedValuesMap)
    , Offsets(offsets)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ReduceByKeyLookupBase() {}
};

/// \brief Execution object holding lookup info for reduce by key.
///
/// A WorkletReduceByKey needs several arrays to map the current output object
/// to the respective key and group of values. This execution object holds that
/// state.
///
template <typename KeyPortalType, typename IdPortalType, typename IdComponentPortalType>
struct ReduceByKeyLookup : ReduceByKeyLookupBase<IdPortalType, IdComponentPortalType>
{
  using KeyType = typename KeyPortalType::ValueType;

  KeyPortalType UniqueKeys;

  VISKORES_EXEC_CONT
  ReduceByKeyLookup(const KeyPortalType& uniqueKeys,
                    const IdPortalType& sortedValuesMap,
                    const IdPortalType& offsets)
    : ReduceByKeyLookupBase<IdPortalType, IdComponentPortalType>(sortedValuesMap, offsets)
    , UniqueKeys(uniqueKeys)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ReduceByKeyLookup() {}
};
}
}
} // namespace viskores::exec::internal

#endif //viskores_exec_internal_ReduceByKeyLookup_h
