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
#ifndef viskores_filter_field_transform_CompositeVectors_h
#define viskores_filter_field_transform_CompositeVectors_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{

/// @brief Combine multiple scalar fields into a single vector field.
///
/// Scalar fields are selected as the active input fields, and the combined vector
/// field is set at the output. The `SetFieldNameList()` method takes a `std::vector`
/// of field names to use as the component fields. Alternately, the `SetActiveField()`
/// method can be used to select the fields independently.
///
/// All of the input fields must be scalar values. The type of the first field
/// determines the type of the output vector field.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT CompositeVectors : public viskores::filter::Filter
{

public:
  VISKORES_CONT
  CompositeVectors() { this->SetOutputFieldName("CompositedField"); };

  /// @brief Specifies the names of the fields to use as components for the output.
  VISKORES_CONT void SetFieldNameList(
    const std::vector<std::string>& fieldNameList,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any);

  /// @brief The number of fields specified as inputs.
  ///
  /// This will be the number of components in the generated field.
  VISKORES_CONT viskores::IdComponent GetNumberOfFields() const;

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
} // namespace field_transform
} // namespace viskores::filter
} // namespace viskores

#endif //viskores_filter_field_transform_CompositeVectors_h
