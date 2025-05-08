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

#ifndef viskores_filter_entity_extraction_Mask_h
#define viskores_filter_entity_extraction_Mask_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/entity_extraction/viskores_filter_entity_extraction_export.h>

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
/// \brief Subselect cells using a stride
///
/// Extract only every Nth cell where N is equal to a stride value
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT Mask : public viskores::filter::Filter
{
public:
  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VISKORES_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VISKORES_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // Set the stride of the subsample
  VISKORES_CONT
  viskores::Id GetStride() const { return this->Stride; }
  VISKORES_CONT
  void SetStride(viskores::Id& stride) { this->Stride = stride; }

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Id Stride = 1;
  bool CompactPoints = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace vtk

#endif // viskores_filter_entity_extraction_Mask_h
