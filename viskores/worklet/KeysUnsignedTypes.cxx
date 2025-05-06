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

#define viskores_worklet_Keys_cxx
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/Keys.hxx>

#define VISKORES_KEYS_EXPORT(T)                                                                \
  template class VISKORES_WORKLET_EXPORT viskores::worklet::Keys<T>;                           \
  template VISKORES_WORKLET_EXPORT VISKORES_CONT void viskores::worklet::Keys<T>::BuildArrays( \
    const viskores::cont::ArrayHandle<T>& keys,                                                \
    viskores::worklet::KeysSortType sort,                                                      \
    viskores::cont::DeviceAdapterId device)

VISKORES_KEYS_EXPORT(viskores::HashType);
VISKORES_KEYS_EXPORT(viskores::UInt8);
using Pair_UInt8_Id2 = viskores::Pair<viskores::UInt8, viskores::Id2>;
VISKORES_KEYS_EXPORT(Pair_UInt8_Id2);


#undef VISKORES_KEYS_EXPORT
