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

#ifndef viskores_filter_density_estimate_ParticleDensityBase_h
#define viskores_filter_density_estimate_ParticleDensityBase_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/density_estimate/viskores_filter_density_estimate_export.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
class VISKORES_FILTER_DENSITY_ESTIMATE_EXPORT ParticleDensityBase : public viskores::filter::Filter
{
protected:
  ParticleDensityBase() = default;

public:
  /// @brief Toggles between summing mass and computing instances.
  ///
  /// When this flag is false (the default), the active field of the input is accumulated
  /// in each bin of the output. When this flag is set to true, the active field is ignored
  /// and the associated particles are simply counted.
  VISKORES_CONT void SetComputeNumberDensity(bool flag) { this->ComputeNumberDensity = flag; }
  /// @copydoc SetComputeNumberDensity
  VISKORES_CONT bool GetComputeNumberDensity() const { return this->ComputeNumberDensity; }

  /// @brief Specifies whether the accumulated mass (or count) is divided by the volume of the cell.
  ///
  /// When this flag is on (the default), the computed mass will be divided by the volume of the
  /// bin to give a density value. Turning off this flag provides an accumulated mass or count.
  ///
  VISKORES_CONT void SetDivideByVolume(bool flag) { this->DivideByVolume = flag; }
  /// @copydoc SetDivideByVolume
  VISKORES_CONT bool GetDivideByVolume() const { return this->DivideByVolume; }

  /// @brief The number of bins in the grid used as regions to estimate density.
  ///
  /// To estimate particle density, this filter defines a uniform grid in space.
  ///
  /// The numbers specify the number of *bins* (i.e. cells in the output mesh) in each
  /// dimension, not the number of points in the output mesh.
  ///
  VISKORES_CONT void SetDimension(const viskores::Id3& dimension) { this->Dimension = dimension; }
  /// @copydoc SetDimension
  VISKORES_CONT viskores::Id3 GetDimension() const { return this->Dimension; }

  /// @brief The lower-left (minimum) corner of the domain of density estimation.
  ///
  VISKORES_CONT void SetOrigin(const viskores::Vec3f& origin) { this->Origin = origin; }
  /// @copydoc SetOrigin
  VISKORES_CONT viskores::Vec3f GetOrigin() const { return this->Origin; }

  /// @brief The spacing of the grid points used to form the grid for density estimation.
  ///
  VISKORES_CONT void SetSpacing(const viskores::Vec3f& spacing) { this->Spacing = spacing; }
  /// @copydoc SetSpacing
  VISKORES_CONT viskores::Vec3f GetSpacing() const { return this->Spacing; }

  /// @brief The bounds of the region where density estimation occurs.
  ///
  /// This method can be used in place of `SetOrigin` and `SetSpacing`. It is often
  /// easiest to compute the bounds of the input coordinate system (or other spatial
  /// region) to use as the input.
  ///
  /// The dimensions must be set before the bounds are set. Calling `SetDimension`
  /// will change the ranges of the bounds.
  ///
  VISKORES_CONT void SetBounds(const viskores::Bounds& bounds)
  {
    this->Origin = { static_cast<viskores::FloatDefault>(bounds.X.Min),
                     static_cast<viskores::FloatDefault>(bounds.Y.Min),
                     static_cast<viskores::FloatDefault>(bounds.Z.Min) };
    this->Spacing = (viskores::Vec3f{ static_cast<viskores::FloatDefault>(bounds.X.Length()),
                                      static_cast<viskores::FloatDefault>(bounds.Y.Length()),
                                      static_cast<viskores::FloatDefault>(bounds.Z.Length()) } /
                     Dimension);
  }
  VISKORES_CONT viskores::Bounds GetBounds() const
  {
    return { { this->Origin[0], this->Origin[0] + (this->Spacing[0] * this->Dimension[0]) },
             { this->Origin[1], this->Origin[1] + (this->Spacing[1] * this->Dimension[1]) },
             { this->Origin[2], this->Origin[2] + (this->Spacing[2] * this->Dimension[2]) } };
  }

protected:
  // Note: we are using the paradoxical "const ArrayHandle&" parameter whose content can actually
  // be change by the function.
  VISKORES_CONT void DoDivideByVolume(const viskores::cont::UnknownArrayHandle& array) const;

  viskores::Id3 Dimension = { 100, 100, 100 }; // Cell dimension
  viskores::Vec3f Origin = { 0.0f, 0.0f, 0.0f };
  viskores::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  bool ComputeNumberDensity = false;
  bool DivideByVolume = true;
};
} // namespace density_estimate
} // namespace filter
} // namespace viskores

#endif //viskores_filter_density_estimate_ParticleDensityBase_h
