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

#ifndef viskores_filter_vector_analysis_VectorMagnitude_h
#define viskores_filter_vector_analysis_VectorMagnitude_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/vector_analysis/viskores_filter_vector_analysis_export.h>

namespace viskores
{
namespace filter
{
namespace vector_analysis
{

/// @brief Compute the magnitudes of a vector field.
///
/// The vector field is selected with the `SetActiveField()` method. The default
/// name for the output field is ``magnitude``, but that can be overridden using
/// the `SetOutputFieldName()` method.
///
class VISKORES_FILTER_VECTOR_ANALYSIS_EXPORT VectorMagnitude : public viskores::filter::Filter
{
public:
  VectorMagnitude();

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace vector_analysis
} // namespace filter
} // namespace viskores::filter

#endif // viskores_filter_vector_analysis_VectorMagnitude_h
