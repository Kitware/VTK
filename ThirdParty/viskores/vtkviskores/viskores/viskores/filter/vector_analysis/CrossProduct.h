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

#ifndef viskores_filter_vector_analysis_CrossProduct_h
#define viskores_filter_vector_analysis_CrossProduct_h

#include <viskores/filter/Filter.h>

#include <viskores/filter/vector_analysis/viskores_filter_vector_analysis_export.h>

namespace viskores
{
namespace filter
{
namespace vector_analysis
{

/// @brief Compute the cross product of 3D vector fields.
///
/// The left part of the operand is the "primary" field and the right part of the operand
/// is the "secondary" field.
class VISKORES_FILTER_VECTOR_ANALYSIS_EXPORT CrossProduct : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  CrossProduct();

  /// @brief Specify the primary field to operate on.
  ///
  /// In the cross product operation A x B, A is the primary field.
  ///
  /// The primary field is an alias for active field index 0. As with any active field,
  /// it can be set as a named field or as a coordinate system.
  VISKORES_CONT void SetPrimaryField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(name, association);
  }

  /// @copydoc SetPrimaryField
  VISKORES_CONT const std::string& GetPrimaryFieldName() const
  {
    return this->GetActiveFieldName();
  }
  /// @copydoc SetPrimaryField
  VISKORES_CONT viskores::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }

  /// @copydoc SetPrimaryField
  VISKORES_CONT void SetUseCoordinateSystemAsPrimaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(flag);
  }
  /// @copydoc SetPrimaryField
  VISKORES_CONT bool GetUseCoordinateSystemAsPrimaryField() const
  {
    return this->GetUseCoordinateSystemAsField();
  }

  /// @copydoc SetPrimaryField
  VISKORES_CONT
  void SetPrimaryCoordinateSystem(viskores::Id index) { this->SetActiveCoordinateSystem(index); }
  VISKORES_CONT viskores::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }

  /// @brief Specify the secondary field to operate on.
  ///
  /// In the cross product operation A x B, B is the secondary field.
  ///
  /// The secondary field is an alias for the active field index 1. As with any active field,
  /// it can be set as a named field or as a coordinate system.
  VISKORES_CONT void SetSecondaryField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(1, name, association);
  }

  /// @copydoc SetSecondaryField
  VISKORES_CONT
  const std::string& GetSecondaryFieldName() const { return this->GetActiveFieldName(1); }
  /// @copydoc SetSecondaryField
  VISKORES_CONT viskores::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }

  /// @copydoc SetSecondaryField
  VISKORES_CONT void SetUseCoordinateSystemAsSecondaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(1, flag);
  }
  /// @copydoc SetSecondaryField
  VISKORES_CONT bool GetUseCoordinateSystemAsSecondaryField() const
  {
    return this->GetUseCoordinateSystemAsField(1);
  }

  /// @copydoc SetSecondaryField
  VISKORES_CONT
  void SetSecondaryCoordinateSystem(viskores::Id index)
  {
    this->SetActiveCoordinateSystem(1, index);
  }
  /// @copydoc SetSecondaryField
  VISKORES_CONT viskores::Id GetSecondaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex(1);
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace vector_analysis
} // namespace filter
} // namespace viskores

#endif // viskores_filter_vector_analysis_CrossProduct_h
