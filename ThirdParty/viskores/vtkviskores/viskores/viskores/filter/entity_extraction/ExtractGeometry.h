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

#ifndef viskores_fulter_entity_extraction_ExtractGeometry_h
#define viskores_fulter_entity_extraction_ExtractGeometry_h

#include <viskores/ImplicitFunction.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/entity_extraction/viskores_filter_entity_extraction_export.h>

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
/// @brief Extract a subset of geometry based on an implicit function
///
/// Extracts from its input geometry all cells that are either
/// completely inside or outside of a specified implicit function. Any type of
/// data can be input to this filter.
///
/// To use this filter you must specify an implicit function. You must also
/// specify whether to extract cells laying inside or outside of the implicit
/// function. (The inside of an implicit function is the negative values
/// region.) An option exists to extract cells that are neither inside or
/// outside (i.e., boundary).
///
/// This differs from `viskores::filter::contour::ClipWithImplicitFunction` in that
/// `viskores::filter::contour::ClipWithImplicitFunction` will subdivide boundary
/// cells into new cells whereas this filter will not, producing a more "crinkly"
/// output.
///
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT ExtractGeometry : public viskores::filter::Filter
{
public:
  /// @brief Specifies the implicit function to be used to perform extract geometry.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `viskores::ImplicitFunctionGeneral` for information on which ones.
  ///
  VISKORES_CONT
  void SetImplicitFunction(const viskores::ImplicitFunctionGeneral& func) { this->Function = func; }

  VISKORES_CONT
  const viskores::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  /// @brief Specify the region of the implicit function to keep cells.
  ///
  /// Determines whether to extract the geometry that is on the inside of the implicit
  /// function (where the function is less than 0) or the outside (where the function is
  /// greater than 0). This flag is true by default (i.e., the interior of the implicit
  /// function will be extracted).
  VISKORES_CONT
  bool GetExtractInside() const { return this->ExtractInside; }
  /// @copydoc GetExtractInside
  VISKORES_CONT
  void SetExtractInside(bool value) { this->ExtractInside = value; }
  /// @copydoc GetExtractInside
  VISKORES_CONT
  void ExtractInsideOn() { this->ExtractInside = true; }
  /// @copydoc GetExtractInside
  VISKORES_CONT
  void ExtractInsideOff() { this->ExtractInside = false; }

  /// @brief Specify whether cells on the boundary should be extracted.
  ///
  /// The implicit function used to extract geometry is likely to intersect some of the
  /// cells of the input. If this flag is true, then any cells intersected by the implicit
  /// function are extracted and included in the output. This flag is false by default.
  VISKORES_CONT
  bool GetExtractBoundaryCells() const { return this->ExtractBoundaryCells; }
  /// @copydoc GetExtractBoundaryCells
  VISKORES_CONT
  void SetExtractBoundaryCells(bool value) { this->ExtractBoundaryCells = value; }
  /// @copydoc GetExtractBoundaryCells
  VISKORES_CONT
  void ExtractBoundaryCellsOn() { this->ExtractBoundaryCells = true; }
  /// @copydoc GetExtractBoundaryCells
  VISKORES_CONT
  void ExtractBoundaryCellsOff() { this->ExtractBoundaryCells = false; }

  /// @brief Specify whether to extract cells only on the boundary.
  ///
  /// When this flag is off (the default), this filter extract the geometry in
  /// the region specified by the implicit function. When this flag is on, then
  /// only those cells that intersect the surface of the implicit function are
  /// extracted.
  VISKORES_CONT
  bool GetExtractOnlyBoundaryCells() const { return this->ExtractOnlyBoundaryCells; }
  /// @brief GetExtractOnlyBoundaryCells
  VISKORES_CONT
  void SetExtractOnlyBoundaryCells(bool value) { this->ExtractOnlyBoundaryCells = value; }
  /// @brief GetExtractOnlyBoundaryCells
  VISKORES_CONT
  void ExtractOnlyBoundaryCellsOn() { this->ExtractOnlyBoundaryCells = true; }
  /// @brief GetExtractOnlyBoundaryCells
  VISKORES_CONT
  void ExtractOnlyBoundaryCellsOff() { this->ExtractOnlyBoundaryCells = false; }

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  bool ExtractInside = true;
  bool ExtractBoundaryCells = false;
  bool ExtractOnlyBoundaryCells = false;
  viskores::ImplicitFunctionGeneral Function;
};
} // namespace entity_extraction
} // namespace filter
} // namespace viskores

#endif // viskores_fulter_entity_extraction_ExtractGeometry_h
