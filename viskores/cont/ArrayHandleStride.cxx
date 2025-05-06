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

#define viskores_cont_ArrayHandleStride_cxx
#include <viskores/cont/ArrayHandleStride.h>

#include <viskores/cont/UnknownArrayHandle.h>

namespace viskores
{
namespace cont
{

namespace internal
{

template class VISKORES_CONT_EXPORT Storage<char, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::Int8, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::UInt8, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::Int16, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::UInt16, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::Int32, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::UInt32, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::Int64, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::UInt64, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::Float32, StorageTagStride>;
template class VISKORES_CONT_EXPORT Storage<viskores::Float64, StorageTagStride>;

} // namespace internal

template class VISKORES_CONT_EXPORT ArrayHandle<char, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Int8, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::UInt8, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Int16, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::UInt16, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Int32, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::UInt32, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Int64, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::UInt64, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Float32, StorageTagStride>;
template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Float64, StorageTagStride>;

}
} // namespace viskores::cont
