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

#ifndef viskores_filter_entity_extraction_ThresholdPoints_h
#define viskores_filter_entity_extraction_ThresholdPoints_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/entity_extraction/viskores_filter_entity_extraction_export.h>

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT ThresholdPoints : public viskores::filter::Filter
{
public:
  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VISKORES_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VISKORES_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  VISKORES_CONT
  viskores::Float64 GetLowerThreshold() const { return this->LowerValue; }
  VISKORES_CONT
  void SetLowerThreshold(viskores::Float64 value) { this->LowerValue = value; }

  VISKORES_CONT
  viskores::Float64 GetUpperThreshold() const { return this->UpperValue; }
  VISKORES_CONT
  void SetUpperThreshold(viskores::Float64 value) { this->UpperValue = value; }

  VISKORES_CONT
  void SetThresholdBelow(viskores::Float64 value);
  VISKORES_CONT
  void SetThresholdAbove(viskores::Float64 value);
  VISKORES_CONT
  void SetThresholdBetween(viskores::Float64 value1, viskores::Float64 value2);

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  constexpr static int THRESHOLD_BELOW = 0;
  constexpr static int THRESHOLD_ABOVE = 1;
  constexpr static int THRESHOLD_BETWEEN = 2;

  double LowerValue = 0;
  double UpperValue = 0;
  int ThresholdType = THRESHOLD_BETWEEN;

  bool CompactPoints = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace viskores

#endif // viskores_filter_entity_extraction_ThresholdPoints_h
