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
#ifndef viskores_filter_FieldSelection_h
#define viskores_filter_FieldSelection_h

#include <viskores/Pair.h>
#include <viskores/cont/Field.h>

#include <viskores/filter/viskores_filter_core_export.h>

#include <initializer_list>
#include <memory>

namespace viskores
{
namespace filter
{

/// A \c FieldSelection stores information about fields to map for input dataset to output
/// when a filter is executed. A \c FieldSelection object is passed to
/// `viskores::filter::Filter::Execute` to execute the filter and map selected
/// fields. It is possible to easily construct \c FieldSelection that selects all or
/// none of the input fields.
class VISKORES_FILTER_CORE_EXPORT FieldSelection
{
public:
  enum struct Mode
  {
    None,
    All,
    Select,
    Exclude
  };

  VISKORES_CONT FieldSelection(Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given a single field name
  /// \code{cpp}
  /// FieldSelection("field_name");
  /// \endcode
  VISKORES_CONT FieldSelection(const std::string& field, Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given a single field name
  /// \code{cpp}
  /// FieldSelection("field_name");
  /// \endcode
  VISKORES_CONT FieldSelection(const char* field, Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given a single name and association.
  /// \code{cpp}
  /// FieldSelection("field_name", viskores::cont::Field::Association::Points)
  /// \endcode{cpp}
  VISKORES_CONT FieldSelection(const std::string& field,
                               viskores::cont::Field::Association association,
                               Mode mode = Mode::Select);

  /// Use this constructor to create a field selection given the field names.
  /// \code{cpp}
  /// FieldSelection({"field_one", "field_two"});
  /// \endcode
  VISKORES_CONT FieldSelection(std::initializer_list<std::string> fields, Mode mode = Mode::Select);

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// using pair_type = std::pair<std::string, viskores::cont::Field::Association>;
  /// FieldSelection({
  ///      pair_type{"field_one", viskores::cont::Field::Association::Points},
  ///      pair_type{"field_two", viskores::cont::Field::Association::Cells} });
  /// @endcode
  VISKORES_CONT FieldSelection(
    std::initializer_list<std::pair<std::string, viskores::cont::Field::Association>> fields,
    Mode mode = Mode::Select);

  /// Use this constructor create a field selection given the field names and
  /// associations e.g.
  /// @code{cpp}
  /// using pair_type = viskores::Pair<std::string, viskores::cont::Field::Association>;
  /// FieldSelection({
  ///      pair_type{"field_one", viskores::cont::Field::Association::Points},
  ///      pair_type{"field_two", viskores::cont::Field::Association::Cells} });
  /// @endcode
  VISKORES_CONT FieldSelection(
    std::initializer_list<viskores::Pair<std::string, viskores::cont::Field::Association>> fields,
    Mode mode = Mode::Select);

  VISKORES_CONT FieldSelection(const FieldSelection& src);
  VISKORES_CONT FieldSelection(FieldSelection&& rhs);
  VISKORES_CONT FieldSelection& operator=(const FieldSelection& src);
  VISKORES_CONT FieldSelection& operator=(FieldSelection&& rhs);

  VISKORES_CONT ~FieldSelection();

  /// Returns true if the input field should be mapped to the output
  /// dataset.
  VISKORES_CONT
  bool IsFieldSelected(const viskores::cont::Field& inputField) const
  {
    return this->IsFieldSelected(inputField.GetName(), inputField.GetAssociation());
  }

  VISKORES_CONT bool IsFieldSelected(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any) const;

  ///@{
  /// Add fields to select or exclude. If no mode is specified, then the mode will follow
  /// that of `GetMode()`.
  VISKORES_CONT void AddField(const viskores::cont::Field& inputField)
  {
    this->AddField(inputField.GetName(), inputField.GetAssociation(), this->GetMode());
  }

  VISKORES_CONT void AddField(const viskores::cont::Field& inputField, Mode mode)
  {
    this->AddField(inputField.GetName(), inputField.GetAssociation(), mode);
  }

  VISKORES_CONT
  void AddField(
    const std::string& fieldName,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->AddField(fieldName, association, this->GetMode());
  }

  VISKORES_CONT void AddField(const std::string& fieldName, Mode mode)
  {
    this->AddField(fieldName, viskores::cont::Field::Association::Any, mode);
  }

  VISKORES_CONT void AddField(const std::string& fieldName,
                              viskores::cont::Field::Association association,
                              Mode mode);
  ///@}

  ///@{
  /// Returns the mode for a particular field. If the field as been added with `AddField`
  /// (or another means), then this will return `Select` or `Exclude`. If the field has
  /// not been added, `None` will be returned.
  VISKORES_CONT Mode GetFieldMode(const viskores::cont::Field& inputField) const
  {
    return this->GetFieldMode(inputField.GetName(), inputField.GetAssociation());
  }

  VISKORES_CONT Mode GetFieldMode(
    const std::string& fieldName,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any) const;
  ///@}

  /// Returns true if the input field has been added to this selection.
  /// Note that depending on the mode of this selection, the result of HasField
  /// is not necessarily the same as IsFieldSelected. (If the mode is MODE_SELECT,
  /// then the result of the two will be the same.)
  VISKORES_CONT bool HasField(const viskores::cont::Field& inputField) const
  {
    return this->HasField(inputField.GetName(), inputField.GetAssociation());
  }

  VISKORES_CONT bool HasField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any) const
  {
    return (this->GetFieldMode(name, association) != Mode::None);
  }

  /// Clear all fields added using `AddField`.
  VISKORES_CONT void ClearFields();

  /// Gets the mode of the field selection. If `Select` mode is on, then only fields that have a
  /// `Select` mode are considered as selected. (All others are considered unselected.) Calling
  /// `AddField` in this mode will mark it as `Select`. If `Exclude` mode is on, then all fields
  /// are considered selected except those fields with an `Exclude` mode. Calling `AddField` in
  /// this mode will mark it as `Exclude`.
  VISKORES_CONT Mode GetMode() const;

  /// Sets the mode of the field selection. If `Select` mode is on, then only fields that have a
  /// `Select` mode are considered as selected. (All others are considered unselected.) Calling
  /// `AddField` in this mode will mark it as `Select`. If `Exclude` mode is on, then all fields
  /// are considered selected except those fields with an `Exclude` mode. Calling `AddField` in
  /// this mode will mark it as `Exclude`.
  ///
  /// If the mode is set to `None`, then the field modes are cleared and the overall mode is set to
  /// `Select` (meaning none of the fields are initially selected). If the mode is set to `All`,
  /// then the field modes are cleared and the overall mode is set to `Exclude` (meaning all of the
  /// fields are initially selected).
  VISKORES_CONT void SetMode(Mode val);

private:
  struct InternalStruct;
  std::unique_ptr<InternalStruct> Internals;
};

}
} // namespace viskores::filter

#endif // viskores_filter_FieldSelection_h
