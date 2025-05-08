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
#ifndef viskores_cont_StorageList_h
#define viskores_cont_StorageList_h

#include <viskores/List.h>

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

namespace viskores
{
namespace cont
{

using StorageListBasic = viskores::List<viskores::cont::StorageTagBasic>;

using StorageListCommon =
  viskores::List<viskores::cont::StorageTagBasic,
                 viskores::cont::StorageTagSOA,
                 viskores::cont::StorageTagUniformPoints,
                 viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                                            viskores::cont::StorageTagBasic,
                                                            viskores::cont::StorageTagBasic>>;

}
} // namespace viskores::cont

#endif //viskores_cont_StorageList_h
