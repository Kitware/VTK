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
#ifndef viskores_filter_contour_ClipWithImplicitFunction_h
#define viskores_filter_contour_ClipWithImplicitFunction_h

#include <viskores/ImplicitFunction.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// \brief Clip a dataset using an implicit function
///
/// Clip a dataset using a given implicit function value, such as `viskores::Sphere`
/// or `viskores::Frustum`. The implicit function uses the point coordinates as its values.
/// If there is more than one coordinate system in the input `viskores::cont::DataSet`,
/// it can be selected with `SetActiveCoordinateSystem()`.
class VISKORES_FILTER_CONTOUR_EXPORT ClipWithImplicitFunction : public viskores::filter::Filter
{
public:
  /// @brief Specifies the implicit function to be used to perform the clip operation.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `viskores::ImplicitFunctionGeneral` for information on which ones.
  ///
  void SetImplicitFunction(const viskores::ImplicitFunctionGeneral& func) { this->Function = func; }

  void SetOffset(viskores::Float64 offset) { this->Offset = offset; }
  viskores::Float64 GetOffset() const { return this->Offset; }

  /// @brief Specifies whether the result of the clip filter should be inverted.
  ///
  /// If set to false (the default), all regions where the implicit function is negative
  /// will be removed. If set to true, all regions where the implicit function is positive
  /// will be removed.
  ///
  void SetInvertClip(bool invert) { this->Invert = invert; }

  /// @brief Specifies the implicit function to be used to perform the clip operation.
  const viskores::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::ImplicitFunctionGeneral Function;
  viskores::Float64 Offset = 0.0;
  bool Invert = false;
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_ClipWithImplicitFunction_h
