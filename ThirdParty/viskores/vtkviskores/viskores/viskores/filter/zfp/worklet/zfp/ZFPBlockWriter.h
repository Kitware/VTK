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
#ifndef viskores_worklet_zfp_block_writer_h
#define viskores_worklet_zfp_block_writer_h

#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

using Word = viskores::UInt64;

template <int block_size, typename AtomicPortalType>
struct BlockWriter
{
  union UIntInt
  {
    viskores::UInt64 uintpart;
    viskores::Int64 intpart;
  };

  viskores::Id m_word_index;
  viskores::Int32 m_start_bit;
  viskores::Int32 m_current_bit;
  const int m_maxbits;
  AtomicPortalType& Portal;

  VISKORES_EXEC BlockWriter(AtomicPortalType& portal,
                            const int& maxbits,
                            const viskores::Id& block_idx)
    : m_current_bit(0)
    , m_maxbits(maxbits)
    , Portal(portal)
  {
    m_word_index = (block_idx * maxbits) / viskores::Int32(sizeof(Word) * 8);
    m_start_bit = viskores::Int32((block_idx * maxbits) % viskores::Int32(sizeof(Word) * 8));
  }

  inline VISKORES_EXEC void Add(const viskores::Id index, Word& value)
  {
    UIntInt newval;
    UIntInt old;
    (void)old;
    newval.uintpart = value;
    Portal.Add(index, newval.intpart);
  }

  inline VISKORES_EXEC viskores::UInt64 write_bits(const viskores::UInt64& bits,
                                                   const unsigned int& n_bits)
  {
    const int wbits = sizeof(Word) * 8;
    unsigned int seg_start = (m_start_bit + m_current_bit) % wbits;
    viskores::Id write_index = m_word_index;
    write_index += viskores::Id((m_start_bit + m_current_bit) / wbits);
    unsigned int seg_end = seg_start + n_bits - 1;
    //int write_index = m_word_index;
    unsigned int shift = seg_start;
    // we may be asked to write less bits than exist in 'bits'
    // so we have to make sure that anything after n is zero.
    // If this does not happen, then we may write into a zfp
    // block not at the specified index
    // uint zero_shift = sizeof(Word) * 8 - n_bits;
    Word left = (bits >> n_bits) << n_bits;

    Word b = bits - left;
    Word add = b << shift;
    Add(write_index, add);

    // n_bits straddles the word boundary
    bool straddle = seg_start < sizeof(Word) * 8 && seg_end >= sizeof(Word) * 8;
    if (straddle)
    {
      Word rem = b >> (sizeof(Word) * 8 - shift);
      Add(write_index + 1, rem);
    }
    m_current_bit += n_bits;
    return bits >> (Word)n_bits;
  }

  // TODO: optimize
  viskores::UInt32 VISKORES_EXEC write_bit(const unsigned int& bit)
  {
    const int wbits = sizeof(Word) * 8;
    unsigned int seg_start = (m_start_bit + m_current_bit) % wbits;
    viskores::Id write_index = m_word_index;
    write_index += viskores::Id((m_start_bit + m_current_bit) / wbits);
    unsigned int shift = seg_start;
    // we may be asked to write less bits than exist in 'bits'
    // so we have to make sure that anything after n is zero.
    // If this does not happen, then we may write into a zfp
    // block not at the specified index
    // uint zero_shift = sizeof(Word) * 8 - n_bits;

    Word add = (Word)bit << shift;
    Add(write_index, add);
    m_current_bit += 1;

    return bit;
  }
};

} // namespace zfp
} // namespace worklet
} // namespace viskores
#endif //  viskores_worklet_zfp_block_writer_h
