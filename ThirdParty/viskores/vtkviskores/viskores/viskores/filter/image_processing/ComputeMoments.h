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
//=======================================================================
#ifndef viskores_filter_image_processing_ComputeMoments_h
#define viskores_filter_image_processing_ComputeMoments_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/image_processing/viskores_filter_image_processing_export.h>

namespace viskores
{
namespace filter
{
namespace image_processing
{
class VISKORES_FILTER_IMAGE_PROCESSING_EXPORT ComputeMoments : public viskores::filter::Filter
{
public:
  VISKORES_CONT ComputeMoments();

  VISKORES_CONT void SetRadius(double _radius) { this->Radius = _radius; }

  VISKORES_CONT void SetSpacing(viskores::Vec3f _spacing) { this->Spacing = _spacing; }

  VISKORES_CONT void SetOrder(viskores::Int32 _order) { this->Order = _order; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  double Radius = 1;
  viskores::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  viskores::Int32 Order = 0;
};
} // namespace image_processing
} // namespace filter
} // namespace viskores

#endif //viskores_filter_image_processing_ComputeMoments_h
