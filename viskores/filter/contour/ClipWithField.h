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

#ifndef viskores_filter_contour_ClipWithField_h
#define viskores_filter_contour_ClipWithField_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// \brief Clip a dataset using a field
///
/// Clip a dataset using a given field value. All points that are less than that
/// value are considered outside, and will be discarded. All points that are greater
/// are kept.
///
/// To select the scalar field, use the `SetActiveField()` and related methods.
///
class VISKORES_FILTER_CONTOUR_EXPORT ClipWithField : public viskores::filter::Filter
{
public:
  /// @brief Specifies the field value for the clip operation.
  ///
  /// Regions where the active field is less than this value are clipped away
  /// from each input cell.
  VISKORES_CONT void SetClipValue(viskores::Float64 value) { this->ClipValue = value; }

  /// @brief Specifies if the result for the clip filter should be inverted.
  ///
  /// If set to false (the default), regions where the active field is less than
  /// the specified clip value are removed. If set to true, regions where the active
  /// field is more than the specified clip value are removed.
  VISKORES_CONT void SetInvertClip(bool invert) { this->Invert = invert; }

  /// @brief Specifies the field value for the clip operation.
  VISKORES_CONT viskores::Float64 GetClipValue() const { return this->ClipValue; }

  /// @brief Specifies if the result for the clip filter should be inverted.
  VISKORES_CONT bool GetInvertClip() const { return this->Invert; }

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Float64 ClipValue = 0;
  bool Invert = false;
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_ClipWithField_h
