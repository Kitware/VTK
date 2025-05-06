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

#ifndef viskores_filter_entity_extraction_ExtractPoints_h
#define viskores_filter_entity_extraction_ExtractPoints_h

#include <viskores/ImplicitFunction.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/entity_extraction/viskores_filter_entity_extraction_export.h>

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
/// @brief Extract only points from a geometry using an implicit function
///
/// Extract only the  points that are either inside or outside of a
/// Viskores implicit function. Examples include planes, spheres, boxes,
/// etc.
///
/// Note that while any geometry type can be provided as input, the output is
/// represented by an explicit representation of points using
/// `viskores::cont::CellSetSingleType` with one vertex cell per point.
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT ExtractPoints : public viskores::filter::Filter
{
public:
  /// @brief Option to remove unused points and compact result int a smaller array.
  ///
  /// When CompactPoints is on, instead of copying the points and point fields
  /// from the input, the filter will create new compact fields without the
  /// unused elements.
  /// When off (the default), unused points will remain listed in the topology,
  /// but point fields and coordinate systems will be shallow-copied to the output.
  VISKORES_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  /// @copydoc GetCompactPoints
  VISKORES_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  /// @brief Specifies the implicit function to be used to perform extract points.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `viskores::ImplicitFunctionGeneral` for information on which ones.
  ///
  void SetImplicitFunction(const viskores::ImplicitFunctionGeneral& func) { this->Function = func; }

  const viskores::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  /// @brief Specify the region of the implicit function to keep points.
  ///
  /// Determines whether to extract the points that are on the inside of the implicit
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

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  bool ExtractInside = true;
  viskores::ImplicitFunctionGeneral Function;

  bool CompactPoints = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace viskores

#endif // viskores_filter_entity_extraction_ExtractPoints_h
