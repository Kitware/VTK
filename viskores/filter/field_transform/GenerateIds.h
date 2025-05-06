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
#ifndef viskores_filter_field_transform_GenerateIds_h
#define viskores_filter_field_transform_GenerateIds_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
/// \brief Adds fields to a `viskores::cont::DataSet` that give the ids for the points and cells.
///
/// This filter will add (by default) a point field named `pointids` that gives the
/// index of the associated point and likewise a cell field named `cellids` for the
/// associated cell indices. These fields are useful for tracking the provenance of
/// the elements of a `viskores::cont::DataSet` as it gets manipulated by filters. It is also
/// convenient for adding indices to operations designed for fields and generally
/// creating test data.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT GenerateIds : public viskores::filter::Filter
{
  std::string PointFieldName = "pointids";
  std::string CellFieldName = "cellids";
  bool GeneratePointIds = true;
  bool GenerateCellIds = true;
  bool UseFloat = false;

public:
  /// @brief The name given to the generated point field.
  ///
  /// By default, the name is `pointids`.
  ///
  const std::string& GetPointFieldName() const { return this->PointFieldName; }
  /// @copydoc GetPointFieldName
  void SetPointFieldName(const std::string& name) { this->PointFieldName = name; }

  /// @brief The name given to the generated cell field.
  ///
  /// By default, the name is `cellids`.
  ///
  const std::string& GetCellFieldName() const { return this->CellFieldName; }
  /// @copydoc GetCellFieldName
  void SetCellFieldName(const std::string& name) { this->CellFieldName = name; }

  /// @brief Specify whether the point id field is generated.
  ///
  /// When `GeneratePointIds` is `true` (the default), a field echoing the point
  /// indices is generated. When set to `false`, this output is not created.
  ///
  bool GetGeneratePointIds() const { return this->GeneratePointIds; }
  /// @copydoc GetGeneratePointIds
  void SetGeneratePointIds(bool flag) { this->GeneratePointIds = flag; }

  /// \brief Specify whether the cell id field is generated.
  ///
  /// When `GenerateCellIds` is `true` (the default), a field echoing the cell
  /// indices is generated. When set to `false`, this output is not created.
  ///
  bool GetGenerateCellIds() const { return this->GenerateCellIds; }
  /// @copydoc GetGenerateCellIds
  void SetGenerateCellIds(bool flag) { this->GenerateCellIds = flag; }

  /// \brief Specify whether the generated fields should be integer or float.
  ///
  /// When `UseFloat` is `false` (the default), then the fields generated will have
  /// type `viskores::Id`. If it is set to `true`, then the fields will be generated
  /// with type `viskores::FloatDefault`.
  ///
  bool GetUseFloat() const { return this->UseFloat; }
  /// @copydoc GetUseFloat
  void SetUseFloat(bool flag) { this->UseFloat = flag; }

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};
} // namespace field_transform
} // namespace viskores::filter
} // namespace viskores

#endif //viskores_filter_field_transform_GenerateIds_h
