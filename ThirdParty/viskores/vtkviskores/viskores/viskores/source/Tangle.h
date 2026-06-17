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

#ifndef viskores_source_Tangle_h
#define viskores_source_Tangle_h

#include <viskores/source/Source.h>

namespace viskores
{
namespace source
{
/**
 * @brief The Tangle source creates a uniform dataset.
 *
 * This class generates a predictable uniform grid dataset with an
 * interesting point field, which is useful for testing and
 * benchmarking.
 *
 * The Execute method creates a complete structured dataset of a
 * resolution specified in the constructor that is bounded by the
 * cube in the range [0,1] in each dimension. The dataset has a
 * point field named 'tangle' computed with the following formula
 *
 * x^4 - 5x^2 + y^4 - 5y^2 + z^4 - 5z^2
 *
**/
class VISKORES_SOURCE_EXPORT Tangle final : public viskores::source::Source
{
public:
  /// \brief Constructs a tangle source with the default point dimensions.
  ///
  /// The default point dimensions are ``{16, 16, 16}``, which produces
  /// ``{15, 15, 15}`` cells.
  VISKORES_CONT Tangle() = default;
  VISKORES_CONT ~Tangle() = default;

  VISKORES_DEPRECATED(2.0, "Use SetCellDimensions or SetPointDimensions.")
  VISKORES_CONT Tangle(viskores::Id3 dims)
    : PointDimensions(dims + viskores::Id3(1))
  {
  }

  /// \brief Gets the number of points in each dimension.
  ///
  /// The generated structured data set has one fewer cell than point in each
  /// dimension.
  VISKORES_CONT viskores::Id3 GetPointDimensions() const { return this->PointDimensions; }

  /// \brief Sets the number of points in each dimension.
  ///
  /// The dimensions must be greater than 1 in each direction to generate cells.
  VISKORES_CONT void SetPointDimensions(viskores::Id3 dims) { this->PointDimensions = dims; }

  /// \brief Gets the number of cells in each dimension.
  ///
  /// This value is computed from the point dimensions by subtracting 1 in each
  /// direction.
  VISKORES_CONT viskores::Id3 GetCellDimensions() const
  {
    return this->PointDimensions - viskores::Id3(1);
  }

  /// \brief Sets the number of cells in each dimension.
  ///
  /// The point dimensions are set to one more than the given cell dimensions in
  /// each direction.
  VISKORES_CONT void SetCellDimensions(viskores::Id3 dims)
  {
    this->PointDimensions = dims + viskores::Id3(1);
  }

private:
  viskores::cont::DataSet DoExecute() const override;

  viskores::Id3 PointDimensions = { 16, 16, 16 };
};
} //namespace source
} //namespace viskores

#endif //viskores_source_Tangle_h
