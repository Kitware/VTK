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
#ifndef viskores_filter_density_estimate_NDEntropy_h
#define viskores_filter_density_estimate_NDEntropy_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/density_estimate/viskores_filter_density_estimate_export.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
/// \brief Calculate the entropy of input N-Dims fields
///
/// This filter calculate the entropy of input N-Dims fields.
///
class VISKORES_FILTER_DENSITY_ESTIMATE_EXPORT NDEntropy : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  void AddFieldAndBin(const std::string& fieldName, viskores::Id numOfBins);

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  std::vector<viskores::Id> NumOfBins;
  std::vector<std::string> FieldNames;
};
} // namespace density_estimate
} // namespace filter
} // namespace viskores

#endif //viskores_filter_density_estimate_NDEntropy_h
