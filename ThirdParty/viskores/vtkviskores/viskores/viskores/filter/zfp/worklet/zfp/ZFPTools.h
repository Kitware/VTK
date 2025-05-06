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
#ifndef viskores_worklet_zfp_tool_h
#define viskores_worklet_zfp_tool_h

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/AtomicArray.h>
#include <viskores/cont/Timer.h>
#include <viskores/worklet/DispatcherMapField.h>

#include <viskores/filter/zfp/worklet/zfp/ZFPEncode3.h>

using ZFPWord = viskores::UInt64;

#include <stdio.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{
namespace detail
{

class MemTransfer : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  MemTransfer() {}
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename PortalType>
  VISKORES_EXEC void operator()(const viskores::Id id, PortalType& outValue) const
  {
    (void)id;
    (void)outValue;
  }
}; //class MemTransfer

inline size_t CalcMem3d(const viskores::Id3 dims, const viskores::UInt32 bits_per_block)
{
  const size_t vals_per_block = 64;
  const size_t size = static_cast<size_t>(dims[0] * dims[1] * dims[2]);
  size_t total_blocks = size / vals_per_block;
  const size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

inline size_t CalcMem2d(const viskores::Id2 dims, const viskores::UInt32 bits_per_block)
{
  constexpr size_t vals_per_block = 16;
  const size_t size = static_cast<size_t>(dims[0] * dims[1]);
  size_t total_blocks = size / vals_per_block;
  constexpr size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

inline size_t CalcMem1d(const viskores::Id dims, const viskores::UInt32 bits_per_block)
{
  constexpr size_t vals_per_block = 4;
  const size_t size = static_cast<size_t>(dims);
  size_t total_blocks = size / vals_per_block;
  constexpr size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}


template <typename T>
T* GetVISKORESPointer(viskores::cont::ArrayHandle<T>& handle)
{
  typedef typename viskores::cont::ArrayHandle<T> HandleType;
  typedef
    typename HandleType::template ExecutionTypes<viskores::cont::DeviceAdapterTagSerial>::Portal
      PortalType;
  typedef typename viskores::cont::ArrayPortalToIterators<PortalType>::IteratorType IteratorType;
  IteratorType iter =
    viskores::cont::ArrayPortalToIterators<PortalType>(handle.WritePortal()).GetBegin();
  return &(*iter);
}

template <typename T, typename S>
void DataDump(viskores::cont::ArrayHandle<T, S> handle, std::string fileName)
{

  T* ptr = GetVISKORESPointer(handle);
  viskores::Id osize = handle.GetNumberOfValues();
  FILE* fp = fopen(fileName.c_str(), "wb");
  ;
  if (fp != NULL)
  {
    fwrite(ptr, sizeof(T), static_cast<size_t>(osize), fp);
  }

  fclose(fp);
}


} // namespace detail
} // namespace zfp
} // namespace worklet
} // namespace viskores
#endif //  viskores_worklet_zfp_tools_h
