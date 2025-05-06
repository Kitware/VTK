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
#ifndef viskores_worklet_zfp_block_reader_h
#define viskores_worklet_zfp_block_reader_h

#include <viskores/filter/zfp/worklet/zfp/ZFPTypeInfo.h>

namespace viskores
{
namespace worklet
{
namespace zfp
{

using Word = viskores::UInt64;

template <viskores::Int32 block_size, typename WordsPortalType>
struct BlockReader
{
  const WordsPortalType& Words;
  const viskores::Int32 m_maxbits;
  int m_block_idx;

  viskores::Int32 m_current_bit;
  viskores::Id Index;

  Word m_buffer;
  const viskores::Id MaxIndex;

  VISKORES_EXEC
  BlockReader(const WordsPortalType& words, const int& maxbits, const int& block_idx)
    : Words(words)
    , m_maxbits(maxbits)
    , MaxIndex(words.GetNumberOfValues() - 1)
  {
    Index = viskores::Id(static_cast<size_t>(block_idx * maxbits) / (sizeof(Word) * 8));
    m_buffer = static_cast<Word>(Words.Get(Index));
    m_current_bit = (block_idx * maxbits) % viskores::Int32((sizeof(Word) * 8));

    m_buffer >>= m_current_bit;
    m_block_idx = block_idx;
  }

  inline VISKORES_EXEC unsigned int read_bit()
  {
    viskores::UInt32 bit = viskores::UInt32(m_buffer) & 1u;
    ++m_current_bit;
    m_buffer >>= 1;
    // handle moving into next word
    if (m_current_bit >= viskores::Int32(sizeof(Word) * 8))
    {
      m_current_bit = 0;
      ++Index;
      if (Index > MaxIndex)
        return false;
      m_buffer = static_cast<Word>(Words.Get(Index));
    }
    return bit;
  }


  // note this assumes that n_bits is <= 64
  inline VISKORES_EXEC viskores::UInt64 read_bits(const int& n_bits)
  {
    viskores::UInt64 bits;
    // rem bits will always be positive
    viskores::Int32 rem_bits = viskores::Int32(sizeof(Word) * 8) - m_current_bit;

    viskores::Int32 first_read = viskores::Min(rem_bits, n_bits);
    // first mask
    Word mask = ((Word)1 << ((first_read))) - 1;
    bits = m_buffer & mask;
    m_buffer >>= n_bits;
    m_current_bit += first_read;
    viskores::Int32 next_read = 0;
    if (n_bits >= rem_bits)
    {
      m_current_bit = 0;
      // just read in 0s if someone asks for more bits past the end of the array.
      // not sure what the best way to deal with this i
      Index = viskores::Min(MaxIndex, Index + 1);
      m_buffer = static_cast<Word>(Words.Get(Index));
      next_read = n_bits - first_read;
    }

    // this is basically a no-op when first read constained
    // all the bits. TODO: if we have aligned reads, this could
    // be a conditional without divergence
    mask = ((Word)1 << ((next_read))) - 1;
    bits += (m_buffer & mask) << first_read;
    m_buffer >>= next_read;
    m_current_bit += next_read;
    return bits;
  }

private:
  VISKORES_EXEC BlockReader() {}

}; // block reader

} // namespace zfp
} // namespace worklet
} // namespace viskores
#endif //  viskores_worklet_zfp_block_reader_h
