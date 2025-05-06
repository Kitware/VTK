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

#ifndef viskores_worklet_connectivity_graph_connectivity_h
#define viskores_worklet_connectivity_graph_connectivity_h

#include <viskores/cont/Invoker.h>
#include <viskores/filter/connected_components/worklet/CellSetDualGraph.h>
#include <viskores/filter/connected_components/worklet/InnerJoin.h>
#include <viskores/filter/connected_components/worklet/UnionFind.h>

namespace viskores
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
class GraphGraft : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn start,
                                FieldIn degree,
                                WholeArrayIn ids,
                                AtomicArrayInOut comp);

  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  template <typename InPortalType, typename AtomicCompInOut>
  VISKORES_EXEC void operator()(viskores::Id index,
                                viskores::Id start,
                                viskores::Id degree,
                                const InPortalType& conn,
                                AtomicCompInOut& comp) const
  {
    for (viskores::Id offset = start; offset < start + degree; offset++)
    {
      viskores::Id neighbor = conn.Get(offset);

      // We need to reload thisComp and thatComp every iteration since
      // they might have been changed by Unite() both as a result of
      // attaching one tree to the other or as a result of path compression
      // in findRoot().
      auto thisComp = comp.Get(index);
      auto thatComp = comp.Get(neighbor);

      // Merge the two components one way or the other, the order will
      // be resolved by Unite().
      UnionFind::Unite(comp, thisComp, thatComp);
    }
  }
};
}

// Single pass connected component algorithm from
// Jaiganesh, Jayadharini, and Martin Burtscher.
// "A high-performance connected components implementation for GPUs."
// Proceedings of the 27th International Symposium on High-Performance
// Parallel and Distributed Computing. 2018.
class GraphConnectivity
{
public:
  template <typename InputArrayType, typename OutputArrayType>
  static void Run(const InputArrayType& numIndicesArray,
                  const InputArrayType& indexOffsetsArray,
                  const InputArrayType& connectivityArray,
                  OutputArrayType& componentsOut)
  {
    VISKORES_IS_ARRAY_HANDLE(InputArrayType);
    VISKORES_IS_ARRAY_HANDLE(OutputArrayType);

    using Algorithm = viskores::cont::Algorithm;

    // Initialize the parent pointer to point to the node itself. There are other
    // ways to initialize the parent pointers, for example, a smaller or the minimal
    // neighbor.
    Algorithm::Copy(viskores::cont::ArrayHandleIndex(numIndicesArray.GetNumberOfValues()),
                    componentsOut);

    viskores::cont::Invoker invoke;
    invoke(
      detail::GraphGraft{}, indexOffsetsArray, numIndicesArray, connectivityArray, componentsOut);
    invoke(PointerJumping{}, componentsOut);

    // renumber connected component to the range of [0, number of components).
    Renumber::Run(componentsOut);
  }
};
}
}
}
#endif //viskores_worklet_connectivity_graph_connectivity_h
