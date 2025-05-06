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

#ifndef viskores_worklet_contour_CommonState_h
#define viskores_worklet_contour_CommonState_h

#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace worklet
{
namespace contour
{

struct CommonState
{
  explicit CommonState(bool mergeDuplicates)
    : MergeDuplicatePoints(mergeDuplicates)
  {
  }

  bool MergeDuplicatePoints = true;
  bool GenerateNormals = false;
  viskores::cont::ArrayHandle<viskores::FloatDefault> InterpolationWeights;
  viskores::cont::ArrayHandle<viskores::Id2> InterpolationEdgeIds;
  viskores::cont::ArrayHandle<viskores::Id> CellIdMap;
};
}
}
}

#endif
