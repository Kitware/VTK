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
#ifndef viskores_filter_density_estimate_ContinuousScatterPlot_h
#define viskores_filter_density_estimate_ContinuousScatterPlot_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/density_estimate/viskores_filter_density_estimate_export.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
/// \brief Constructs the continuous scatterplot for two given scalar point fields of a mesh.
///
/// The continuous scatterplot is an extension of the discrete scatterplot for continuous bi-variate analysis.
/// This filter outputs an ExplicitDataSet of triangle-shaped cells, whose coordinates on the 2D plane represent respectively
/// the values of both scalar fields. Triangles' points are associated with a scalar field, representing the
/// density of values in the data domain. The filter tetrahedralizes the input dataset before operating.
///
/// If both fields provided don't have the same floating point precision, the output will
/// have the precision of the first one of the pair.
///
/// This implementation is based on the algorithm presented in the publication :
///
/// S. Bachthaler and D. Weiskopf, "Continuous Scatterplots"
/// in IEEE Transactions on Visualization and Computer Graphics,
/// vol. 14, no. 6, pp. 1428-1435, Nov.-Dec. 2008
/// doi: 10.1109/TVCG.2008.119.

class VISKORES_FILTER_DENSITY_ESTIMATE_EXPORT ContinuousScatterPlot
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT ContinuousScatterPlot() { this->SetOutputFieldName("density"); }

  /// Select both point fields to use when running the filter.
  /// Replaces setting each one individually using `SetActiveField` on indices 0 and 1.
  VISKORES_CONT
  void SetActiveFieldsPair(const std::string& fieldName1, const std::string& fieldName2)
  {
    SetActiveField(0, fieldName1, viskores::cont::Field::Association::Points);
    SetActiveField(1, fieldName2, viskores::cont::Field::Association::Points);
  };

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtm

#endif //viskores_filter_density_estimate_ContinuousScatterPlot_h
