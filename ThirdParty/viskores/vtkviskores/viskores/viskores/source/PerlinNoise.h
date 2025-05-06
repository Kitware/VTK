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

#ifndef viskores_source_PerlinNoise_h
#define viskores_source_PerlinNoise_h

#include <viskores/source/Source.h>

namespace viskores
{
namespace source
{
/**
 * @brief The PerlinNoise source creates a uniform dataset.
 *
 * This class generates a uniform grid dataset with a tileable perlin
 * noise scalar point field.
 *
 * The Execute method creates a complete structured dataset that have a
 * scalar point field named 'perlinnoise'.
**/
class VISKORES_SOURCE_EXPORT PerlinNoise final : public viskores::source::Source
{
public:
  VISKORES_CONT PerlinNoise() = default;
  VISKORES_CONT ~PerlinNoise() = default;

  VISKORES_CONT PerlinNoise(const PerlinNoise&) = default;
  VISKORES_CONT PerlinNoise(PerlinNoise&&) = default;
  VISKORES_CONT PerlinNoise& operator=(const PerlinNoise&) = default;
  VISKORES_CONT PerlinNoise& operator=(PerlinNoise&&) = default;

  VISKORES_DEPRECATED(2.0, "Use SetCellDimensions or SetPointDimensions.")
  VISKORES_CONT PerlinNoise(viskores::Id3 dims);
  VISKORES_DEPRECATED(2.0, "Use Set*Dimensions and SetSeed.")
  VISKORES_CONT PerlinNoise(viskores::Id3 dims, viskores::IdComponent seed);
  VISKORES_DEPRECATED(2.0, "Use Set*Dimensions and SetOrigin.")
  VISKORES_CONT PerlinNoise(viskores::Id3 dims, viskores::Vec3f origin);
  VISKORES_DEPRECATED(2.0, "Use Set*Dimensions, SetOrigin, and SetSeed.")
  VISKORES_CONT PerlinNoise(viskores::Id3 dims, viskores::Vec3f origin, viskores::IdComponent seed);

  VISKORES_CONT viskores::Id3 GetPointDimensions() const { return this->PointDimensions; }
  VISKORES_CONT void SetPointDimensions(viskores::Id3 dims) { this->PointDimensions = dims; }

  VISKORES_CONT viskores::Id3 GetCellDimensions() const
  {
    return this->PointDimensions - viskores::Id3(1);
  }
  VISKORES_CONT void SetCellDimensions(viskores::Id3 dims)
  {
    this->PointDimensions = dims + viskores::Id3(1);
  }

  VISKORES_CONT viskores::Vec3f GetOrigin() const { return this->Origin; }
  VISKORES_CONT void SetOrigin(const viskores::Vec3f& origin) { this->Origin = origin; }

  /// \brief The seed used for the pseudorandom number generation of the noise.
  ///
  /// If the seed is not set, then a new, unique seed is picked each time `Execute` is run.
  VISKORES_CONT viskores::IdComponent GetSeed() const { return this->Seed; }
  VISKORES_CONT void SetSeed(viskores::IdComponent seed)
  {
    this->Seed = seed;
    this->SeedSet = true;
  }

private:
  viskores::cont::DataSet DoExecute() const override;

  viskores::Id3 PointDimensions = { 16, 16, 16 };
  viskores::Vec3f Origin = { 0, 0, 0 };
  viskores::IdComponent Seed = 0;
  bool SeedSet = false;
};
} //namespace source
} //namespace viskores

#endif //viskores_source_PerlinNoise_h
