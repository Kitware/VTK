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
#ifndef viskores_cont_CellSetList_h
#define viskores_cont_CellSetList_h

#include <viskores/List.h>

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>

namespace viskores
{
namespace cont
{

using CellSetListStructured1D = viskores::List<viskores::cont::CellSetStructured<1>>;

using CellSetListStructured2D = viskores::List<viskores::cont::CellSetStructured<2>>;

using CellSetListStructured3D = viskores::List<viskores::cont::CellSetStructured<3>>;


template <typename ShapesStorageTag = VISKORES_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VISKORES_DEFAULT_OFFSETS_STORAGE_TAG>
using CellSetListExplicit = viskores::List<
  viskores::cont::CellSetExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>>;

using CellSetListExplicitDefault = CellSetListExplicit<>;

using CellSetListCommon = viskores::List<viskores::cont::CellSetStructured<2>,
                                         viskores::cont::CellSetStructured<3>,
                                         viskores::cont::CellSetExplicit<>,
                                         viskores::cont::CellSetSingleType<>>;

using CellSetListStructured =
  viskores::List<viskores::cont::CellSetStructured<2>, viskores::cont::CellSetStructured<3>>;

using CellSetListUnstructured =
  viskores::List<viskores::cont::CellSetExplicit<>, viskores::cont::CellSetSingleType<>>;
}
} // namespace viskores::cont

#endif //viskores_cont_CellSetList_h
