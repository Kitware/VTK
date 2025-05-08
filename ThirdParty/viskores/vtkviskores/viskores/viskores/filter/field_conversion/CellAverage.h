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

#ifndef viskores_filter_field_conversion_CellAverage_h
#define viskores_filter_field_conversion_CellAverage_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_conversion/viskores_filter_field_conversion_export.h>

namespace viskores
{
namespace filter
{
namespace field_conversion
{
/// \brief Point to cell interpolation filter.
///
/// CellAverage is a filter that transforms point data (i.e., data
/// specified at cell points) into cell data (i.e., data specified per cell).
/// The method of transformation is based on averaging the data
/// values of all points used by particular cell.
///
/// The point field to convert comes from the active scalars.
/// The default name for the output cell field is the same name as the input
/// point field. The name can be overridden as always using the
/// `SetOutputFieldName()` method.
///
class VISKORES_FILTER_FIELD_CONVERSION_EXPORT CellAverage : public viskores::filter::Filter
{
private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
} // namespace field_conversion
} // namespace filter
} // namespace viskores

#endif // viskores_filter_field_conversion_CellAverage_h
