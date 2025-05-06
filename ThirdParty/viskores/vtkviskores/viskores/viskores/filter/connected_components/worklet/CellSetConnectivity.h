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
#ifndef viskores_worklet_connectivity_CellSetConnectivity_h
#define viskores_worklet_connectivity_CellSetConnectivity_h

#include <viskores/filter/connected_components/worklet/CellSetDualGraph.h>
#include <viskores/filter/connected_components/worklet/GraphConnectivity.h>

namespace viskores
{
namespace worklet
{
namespace connectivity
{

class CellSetConnectivity
{
public:
  static void Run(const viskores::cont::UnknownCellSet& cellSet,
                  viskores::cont::ArrayHandle<viskores::Id>& componentArray)
  {
    viskores::cont::ArrayHandle<viskores::Id> numIndicesArray;
    viskores::cont::ArrayHandle<viskores::Id> indexOffsetsArray;
    viskores::cont::ArrayHandle<viskores::Id> connectivityArray;

    // create cell to cell connectivity graph (dual graph)
    CellSetDualGraph::Run(cellSet, numIndicesArray, indexOffsetsArray, connectivityArray);
    // find the connected component of the dual graph
    GraphConnectivity::Run(numIndicesArray, indexOffsetsArray, connectivityArray, componentArray);
  }
};
}
}
} // viskores::worklet::connectivity

#endif // viskores_worklet_connectivity_CellSetConnectivity_h
