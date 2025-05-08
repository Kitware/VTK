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
#ifndef viskores_CellClassification_h
#define viskores_CellClassification_h

#include <viskores/Types.h>

namespace viskores
{

/// \brief Bit flags used in ghost arrays to identify what type a cell is.
///
/// `CellClassification` contains several bit flags that determine whether a cell is
/// normal or if it should be treated as duplicated or removed in some way. The flags
/// can be (and should be) treated as `viskores::UInt8` and or-ed together.
///
class CellClassification
{
  // Implementation note: An enum would be a natural representation for these flags.
  // However, a non-scoped enum leaks the names into the broader namespace and a
  // scoped enum is too difficult to convert to the `viskores::UInt8` we really want to
  // treat it as. Thus, use constexpr to define the `viskores::UInt8`s.
  viskores::UInt8 Flags;

public:
  // Use an unscoped enum here, where it will be properly scoped in the class.
  // Using unscoped enums in this way is sort of obsolete, but prior to C++17
  // a `static constexpr` may require a definition in a .cxx file, and that is
  // not really possible for this particular class (which could be used on a GPU).
  enum : viskores::UInt8
  {
    Normal = 0,       //Valid cell
    Ghost = 1 << 0,   //Ghost cell
    Invalid = 1 << 1, //Cell is invalid
    Unused0 = 1 << 2,
    Blanked = 1 << 3, //Blanked cell in AMR
    Unused3 = 1 << 4,
    Unused4 = 1 << 5,
    Unused5 = 1 << 6,
  };

  /// @var viskores::UInt8 Normal
  /// @brief Value used for a normal cell.
  ///
  /// This value is the clearing of any cell classification flags. This identifies
  /// the cells as a "normal" cell without any special or exclusionary properties.

  /// @var viskores::UInt8 Ghost
  /// @brief Flag used for a ghost cell.
  ///
  /// This flag indicates the associated cell is repeated information from a different
  /// partition. The ghost cell is provided to give data from cells in neighboring
  /// partitions. This allows operations to correctly compute neighborhood information
  /// without explicit communications. Ghost cells are typically removed for rendering.

  /// @var viskores::UInt8 Invalid
  /// @brief Flag used for an invalid cell.

  /// @var viskores::UInt8 Blanked
  /// @brief Flag used for a cell that should not be considered part of the data.
  ///
  /// A blanked cell should be ignored from the data. Cells with this flag should
  /// be treated as if they were not declared. Blanked cells are primarily used
  /// in structured cell sets to remove parts of the interior of the mesh volume that
  /// could not otherwise be removed. Blanked cells are common in AMR structures
  /// to indicate cells that are further refined in deeper levels.

  VISKORES_EXEC constexpr CellClassification(viskores::UInt8 flags = viskores::UInt8{ Normal })
    : Flags(flags)
  {
  }

  VISKORES_EXEC constexpr operator viskores::UInt8() const { return this->Flags; }
};

} // namespace viskores

#endif // viskores_CellClassification_h
