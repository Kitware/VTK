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

#ifndef viskores_filter_density_estimate_Entropy_h
#define viskores_filter_density_estimate_Entropy_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/density_estimate/viskores_filter_density_estimate_export.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
/// \brief Construct the entropy histogram of a given Field
///
/// Construct a histogram which is used to compute the entropy with a default of 10 bins
///
class VISKORES_FILTER_DENSITY_ESTIMATE_EXPORT Entropy : public viskores::filter::Filter
{
public:
  //currently the Entropy filter only works on scalar data.
  using SupportedTypes = TypeListScalarAll;

  //Construct a histogram which is used to compute the entropy with a default of 10 bins
  VISKORES_CONT
  Entropy();

  VISKORES_CONT
  void SetNumberOfBins(viskores::Id count) { this->NumberOfBins = count; }
  VISKORES_CONT
  viskores::Id GetNumberOfBins() const { return this->NumberOfBins; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Id NumberOfBins = 10;
};
} // namespace density_estimate
} // namespace filter
} // namespace viskores

#endif // viskores_filter_density_estimate_Entropy_h
