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

#ifndef viskores_Bitset_h
#define viskores_Bitset_h

#include <cassert>
#include <limits>
#include <viskores/Types.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{

/// \brief A bitmap to serve different needs.
/// Ex. Editing particular bits in a byte(s), checkint if particular bit values
/// are present or not. Once Cuda supports std::bitset, we should use the
/// standard one if possible. Additional cast in logical operations are required
/// to avoid compiler warnings when using 16 or 8 bit MaskType.
template <typename MaskType>
struct Bitset
{
  VISKORES_EXEC_CONT void set(viskores::Id bitIndex)
  {
    this->Mask = static_cast<MaskType>(this->Mask | (static_cast<MaskType>(1) << bitIndex));
  }

  VISKORES_EXEC_CONT void set(viskores::Id bitIndex, bool val)
  {
    if (val)
      this->set(bitIndex);
    else
      this->reset(bitIndex);
  }

  VISKORES_EXEC_CONT void reset(viskores::Id bitIndex)
  {
    this->Mask = static_cast<MaskType>(this->Mask & ~(static_cast<MaskType>(1) << bitIndex));
  }

  VISKORES_EXEC_CONT void toggle(viskores::Id bitIndex)
  {
    this->Mask = this->Mask ^ (static_cast<MaskType>(0) << bitIndex);
  }

  VISKORES_EXEC_CONT bool test(viskores::Id bitIndex) const
  {
    return ((this->Mask & (static_cast<MaskType>(1) << bitIndex)) != 0);
  }

  VISKORES_EXEC_CONT bool operator==(const viskores::Bitset<MaskType>& otherBitset) const
  {
    return this->Mask == otherBitset.Mask;
  }

private:
  MaskType Mask = 0;
};

} // namespace viskores

#endif //viskores_Bitset_h
