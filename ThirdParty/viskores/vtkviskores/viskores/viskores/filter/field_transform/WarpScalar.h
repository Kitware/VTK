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

#ifndef viskores_filter_field_transform_WarpScalar_h
#define viskores_filter_field_transform_WarpScalar_h

#include <viskores/filter/field_transform/Warp.h>

#include <viskores/Deprecated.h>

struct VISKORES_DEPRECATED(2.2, "WarpScalar.h header no longer supported. Use Warp.h.")
  viskores_deprecated_WarpScalar_h_warning
{
};

viskores_deprecated_WarpScalar_h_warning viskores_give_WarpScalar_h_warning;

namespace viskores
{
namespace filter
{
namespace field_transform
{

class VISKORES_DEPRECATED(2.2, "Use more general Warp filter.") WarpScalar
  : public viskores::filter::field_transform::Warp
{
public:
  VISKORES_DEPRECATED(2.2, "Use SetScaleFactor().")
  VISKORES_CONT explicit WarpScalar(viskores::FloatDefault scaleAmount)
  {
    this->SetScaleFactor(scaleAmount);
    this->SetOutputFieldName("warpscalar");
  }

  VISKORES_DEPRECATED(2.2, "Use SetDirectionField().")
  VISKORES_CONT void SetNormalField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    if ((association != viskores::cont::Field::Association::Any) &&
        (association != viskores::cont::Field::Association::Points))
    {
      throw viskores::cont::ErrorBadValue("Normal field should always be associated with points.");
    }
    this->SetDirectionField(name);
  }

  VISKORES_DEPRECATED(2.2, "Use GetDirectionFieldName().")
  VISKORES_CONT std::string GetNormalFieldName() const { return this->GetDirectionFieldName(); }

  VISKORES_DEPRECATED(2.2, "Only point association supported.")
  VISKORES_CONT viskores::cont::Field::Association GetNormalFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }

  VISKORES_DEPRECATED(2.2, "Use SetScaleField().")
  VISKORES_CONT void SetScalarFactorField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    if ((association != viskores::cont::Field::Association::Any) &&
        (association != viskores::cont::Field::Association::Points))
    {
      throw viskores::cont::ErrorBadValue("Normal field should always be associated with points.");
    }
    this->SetScaleField(name);
  }

  VISKORES_DEPRECATED(2.2, "Use GetScaleField().")
  VISKORES_CONT std::string GetScalarFactorFieldName() const { return this->GetScaleFieldName(); }

  VISKORES_DEPRECATED(2.2, "Only point association supported.")
  VISKORES_CONT viskores::cont::Field::Association GetScalarFactorFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }
};

} // namespace field_transform
} // namespace filter
} // namespace viskores

#endif //viskores_filter_field_transform_WarpScalar_h
