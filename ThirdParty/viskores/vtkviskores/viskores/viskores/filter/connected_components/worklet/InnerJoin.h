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

#ifndef viskores_worklet_connectivity_InnerJoin_h
#define viskores_worklet_connectivity_InnerJoin_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace connectivity
{
class InnerJoin
{
public:
  struct Merge : viskores::worklet::WorkletMapField
  {
    using ControlSignature =
      void(FieldIn, FieldIn, FieldIn, WholeArrayIn, FieldOut, FieldOut, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, VisitIndex, _4, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    // TODO: type trait for array portal?
    template <typename KeyType, typename ValueType1, typename InPortalType, typename ValueType2>
    VISKORES_EXEC void operator()(KeyType key,
                                  ValueType1 value1,
                                  viskores::Id lowerBounds,
                                  viskores::Id visitIndex,
                                  const InPortalType& value2,
                                  viskores::Id& keyOut,
                                  ValueType1& value1Out,
                                  ValueType2& value2Out) const
    {
      auto v2 = value2.Get(lowerBounds + visitIndex);
      keyOut = key;
      value1Out = value1;
      value2Out = v2;
    }
  };

  using Algorithm = viskores::cont::Algorithm;

  // TODO: not mutating input keys and values?
  template <typename Key, typename Value1, typename Value2>
  static void Run(viskores::cont::ArrayHandle<Key>& key1,
                  viskores::cont::ArrayHandle<Value1>& value1,
                  viskores::cont::ArrayHandle<Key>& key2,
                  viskores::cont::ArrayHandle<Value2>& value2,
                  viskores::cont::ArrayHandle<Key>& keyOut,
                  viskores::cont::ArrayHandle<Value1>& value1Out,
                  viskores::cont::ArrayHandle<Value2>& value2Out)
  {
    Algorithm::SortByKey(key1, value1);
    Algorithm::SortByKey(key2, value2);

    viskores::cont::ArrayHandle<viskores::Id> lbs;
    viskores::cont::ArrayHandle<viskores::Id> ubs;
    Algorithm::LowerBounds(key2, key1, lbs);
    Algorithm::UpperBounds(key2, key1, ubs);

    viskores::cont::ArrayHandle<viskores::Id> counts;
    Algorithm::Transform(ubs, lbs, counts, viskores::Subtract());

    viskores::worklet::ScatterCounting scatter{ counts };
    viskores::worklet::DispatcherMapField<Merge> mergeDisp(scatter);
    mergeDisp.Invoke(key1, value1, lbs, value2, keyOut, value1Out, value2Out);
  }
};

class Renumber
{
public:
  static void Run(viskores::cont::ArrayHandle<viskores::Id>& componentsInOut)
  {
    using Algorithm = viskores::cont::Algorithm;

    // FIXME: we should be able to apply findRoot to each pixel and use some kind
    // of atomic operation to get the number of unique components without the
    // cost of copying and sorting. This might be able to be extended to also
    // work for the renumbering (replacing InnerJoin) through atomic increment.
    viskores::cont::ArrayHandle<viskores::Id> uniqueComponents;
    Algorithm::Copy(componentsInOut, uniqueComponents);
    Algorithm::Sort(uniqueComponents);
    Algorithm::Unique(uniqueComponents);

    viskores::cont::ArrayHandle<viskores::Id> ids;
    Algorithm::Copy(viskores::cont::ArrayHandleIndex(componentsInOut.GetNumberOfValues()), ids);

    viskores::cont::ArrayHandle<viskores::Id> uniqueColor;
    Algorithm::Copy(viskores::cont::ArrayHandleIndex(uniqueComponents.GetNumberOfValues()),
                    uniqueColor);

    viskores::cont::ArrayHandle<viskores::Id> cellColors;
    viskores::cont::ArrayHandle<viskores::Id> pixelIdsOut;
    InnerJoin::Run(componentsInOut,
                   ids,
                   uniqueComponents,
                   uniqueColor,
                   cellColors,
                   pixelIdsOut,
                   componentsInOut);

    Algorithm::SortByKey(pixelIdsOut, componentsInOut);
  }
};
}
}
} // viskores::worklet::connectivity

#endif //viskores_worklet_connectivity_InnerJoin_h
