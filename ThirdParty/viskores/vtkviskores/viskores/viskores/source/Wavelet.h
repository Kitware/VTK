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

#ifndef viskores_source_Wavelet_h
#define viskores_source_Wavelet_h

#include <viskores/source/Source.h>

#include <viskores/Math.h>

namespace viskores
{
namespace source
{
/**
 * @brief The Wavelet source creates a dataset similar to VTK's
 * vtkRTAnalyticSource.
 *
 * This class generates a predictable structured dataset with a smooth yet
 * interesting set of scalars, which is useful for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that has a
 * point field named `RTData`
 *
 * The RTData scalars are computed as:
 *
 * ```
 * MaxVal * Gauss + MagX * sin(FrqX*x) + MagY * sin(FrqY*y) + MagZ * cos(FrqZ*z)
 * ```
 *
 * The dataset properties are determined by:
 * - `Minimum/MaximumExtent`: The logical point extents of the dataset.
 * - `Spacing`: The distance between points of the dataset.
 * - `Center`: The center of the dataset.
 *
 * The scalar functions is control via:
 * - `Center`: The center of a Gaussian contribution to the scalars.
 * - `StandardDeviation`: The unscaled width of a Gaussian contribution.
 * - `MaximumValue`: Upper limit of the scalar range.
 * - `Frequency`: The Frq[XYZ] parameters of the periodic contributions.
 * - `Magnitude`: The Mag[XYZ] parameters of the periodic contributions.
 *
 * By default, the following parameters are used:
 * - `Extents`: { -10, -10, -10 } `-->` { 10, 10, 10 }
 * - `Spacing`: { 1, 1, 1 }
 * - `Center`: { 0, 0, 0 }
 * - `StandardDeviation`: 0.5
 * - `MaximumValue`: 255
 * - `Frequency`: { 60, 30, 40 }
 * - `Magnitude`: { 10, 18, 5 }
 *
 *  If the extent has zero length in the z-direction, a 2D dataset is generated.
 */
class VISKORES_SOURCE_EXPORT Wavelet final : public viskores::source::Source
{
public:
  VISKORES_CONT Wavelet() = default;
  VISKORES_CONT ~Wavelet() = default;

  VISKORES_DEPRECATED(2.0, "Use SetExtent.")
  VISKORES_CONT Wavelet(viskores::Id3 minExtent, viskores::Id3 maxExtent = { 10 });

  ///@{
  /// \brief Specifies the center of the wavelet function.
  ///
  /// Note that the center of the function can be anywhere in space including
  /// outside the domain of the data created (as specified by the origin,
  /// spacing and extent).
  VISKORES_CONT void SetCenter(const viskores::Vec3f& center) { this->Center = center; }
  VISKORES_CONT viskores::Vec3f GetCenter() const { return this->Center; }
  ///@}

  ///@{
  /// \brief Specifies the origin (lower left corner) of the dataset created.
  ///
  /// If the origin is not specified, it will be placed such that extent
  /// index (0, 0, 0) is at the coordinate system origin.
  VISKORES_CONT void SetOrigin(const viskores::Vec3f& origin) { this->Origin = origin; }
  VISKORES_CONT viskores::Vec3f GetOrigin() const
  {
    if (!viskores::IsNan(this->Origin[0]))
    {
      return this->Origin;
    }
    else
    {
      return this->MinimumExtent * this->Spacing;
    }
  }

  VISKORES_CONT void SetSpacing(const viskores::Vec3f& spacing) { this->Spacing = spacing; }
  VISKORES_CONT viskores::Vec3f GetSpacing() const { return this->Spacing; }

  VISKORES_CONT void SetFrequency(const viskores::Vec3f& frequency) { this->Frequency = frequency; }
  VISKORES_CONT viskores::Vec3f GetFrequency() const { return this->Frequency; }

  VISKORES_CONT void SetMagnitude(const viskores::Vec3f& magnitude) { this->Magnitude = magnitude; }
  VISKORES_CONT viskores::Vec3f GetMagnitude() const { return this->Magnitude; }

  VISKORES_CONT void SetMinimumExtent(const viskores::Id3& minExtent)
  {
    this->MinimumExtent = minExtent;
  }
  VISKORES_CONT viskores::Id3 GetMinimumExtent() const { return this->MinimumExtent; }

  VISKORES_CONT void SetMaximumExtent(const viskores::Id3& maxExtent)
  {
    this->MaximumExtent = maxExtent;
  }
  VISKORES_CONT viskores::Id3 GetMaximumExtent() const { return this->MaximumExtent; }

  VISKORES_CONT void SetExtent(const viskores::Id3& minExtent, const viskores::Id3& maxExtent)
  {
    this->MinimumExtent = minExtent;
    this->MaximumExtent = maxExtent;
  }

  VISKORES_CONT void SetMaximumValue(const viskores::FloatDefault& maxVal)
  {
    this->MaximumValue = maxVal;
  }
  VISKORES_CONT viskores::FloatDefault GetMaximumValue() const { return this->MaximumValue; }

  VISKORES_CONT void SetStandardDeviation(const viskores::FloatDefault& stdev)
  {
    this->StandardDeviation = stdev;
  }
  VISKORES_CONT viskores::FloatDefault GetStandardDeviation() const
  {
    return this->StandardDeviation;
  }

private:
  viskores::cont::DataSet DoExecute() const override;

  template <viskores::IdComponent Dim>
  viskores::cont::Field GeneratePointField(const viskores::cont::CellSetStructured<Dim>& cellset,
                                           const std::string& name) const;

  template <viskores::IdComponent Dim>
  viskores::cont::DataSet GenerateDataSet(viskores::cont::CoordinateSystem coords) const;

  viskores::Vec3f Center = { 0, 0, 0 };
  viskores::Vec3f Origin = { viskores::Nan<viskores::FloatDefault>() };
  viskores::Vec3f Spacing = { 1, 1, 1 };
  viskores::Vec3f Frequency = { 60.0f, 30.0f, 40.0f };
  viskores::Vec3f Magnitude = { 10.0f, 18.0f, 5.0f };
  viskores::Id3 MinimumExtent = { -10, -10, -10 };
  viskores::Id3 MaximumExtent = { 10, 10, 10 };
  viskores::FloatDefault MaximumValue = 255.0f;
  viskores::FloatDefault StandardDeviation = 0.5f;
};
} //namespace source
} //namespace viskores

#endif //viskores_source_Wavelet_h
