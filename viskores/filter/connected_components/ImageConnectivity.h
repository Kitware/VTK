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

#ifndef viskores_filter_connected_components_ImageConnectivity_h
#define viskores_filter_connected_components_ImageConnectivity_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/connected_components/viskores_filter_connected_components_export.h>

/// \brief Groups connected points that have the same field value.
///
/// The ImageConnectivity filter finds groups of points that have the same field value and are
/// connected together through their topology. Any point is considered to be connected to its Moore neighborhood:
/// 8 neighboring points for 2D and 27 neighboring points for 3D. As the name implies, `ImageConnectivity` only
/// works on data with a structured cell set. You will get an error if you use any other type of cell set.
///
/// The active field passed to the filter must be associated with the points.
///
/// The result of the filter is a point field of type `viskores::Id`. Each entry in the point field will be a number that
/// identifies to which region it belongs. By default, this output point field is named "component"
/// (which can be changed with the `SetOutputFieldName` method).
namespace viskores
{
namespace filter
{
namespace connected_components
{
class VISKORES_FILTER_CONNECTED_COMPONENTS_EXPORT ImageConnectivity
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT ImageConnectivity() { this->SetOutputFieldName("component"); }

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
} // namespace connected_components

} // namespace filter
} // namespace viskores

#endif //viskores_filter_connected_components_ImageConnectivity_h
