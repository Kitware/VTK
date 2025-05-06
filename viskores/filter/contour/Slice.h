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
#ifndef viskores_filter_contour_Slice_h
#define viskores_filter_contour_Slice_h

#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

#include <viskores/ImplicitFunction.h>

namespace viskores
{
namespace filter
{
namespace contour
{

/// @brief Intersect a mesh with an implicit surface.
///
/// This filter accepts a `viskores::ImplicitFunction` that defines the surface to
/// slice on. A `viskores::Plane` is a common function to use that cuts the mesh
/// along a plane.
///
class VISKORES_FILTER_CONTOUR_EXPORT Slice : public viskores::filter::contour::Contour
{
public:
  /// @brief Set the implicit function that is used to perform the slicing.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `viskores::ImplicitFunctionGeneral` for information on which ones.
  ///
  VISKORES_CONT
  void SetImplicitFunction(const viskores::ImplicitFunctionGeneral& func) { this->Function = func; }
  /// @brief Get the implicit function that us used to perform the slicing.
  VISKORES_CONT
  const viskores::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::ImplicitFunctionGeneral Function;
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_Slice_h
