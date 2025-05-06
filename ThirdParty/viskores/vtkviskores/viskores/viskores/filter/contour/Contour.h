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

#ifndef viskores_filter_contour_Contour_h
#define viskores_filter_contour_Contour_h

#include <viskores/filter/contour/AbstractContour.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

namespace viskores
{
namespace filter
{
namespace contour
{

/// \brief Generate contours or isosurfaces from a region of space.
///
/// `Contour` takes as input a mesh, often a volume, and generates on
/// output one or more surfaces where a field equals a specified value.
///
/// This filter implements multiple algorithms for contouring, and the best algorithm
/// will be selected based on the type of the input.
///
/// The scalar field to extract the contour from is selected with the `SetActiveField()`
/// and related methods.
///
class VISKORES_FILTER_CONTOUR_EXPORT Contour : public viskores::filter::contour::AbstractContour
{
public:
  VISKORES_DEPRECATED(2.1, "Use SetComputeFastNormals.")
  VISKORES_CONT void SetComputeFastNormalsForStructured(bool on)
  {
    this->SetComputeFastNormals(on);
  }
  VISKORES_DEPRECATED(2.1, "Use GetComputeFastNormals.")
  VISKORES_CONT bool GetComputeFastNormalsForStructured() const
  {
    return this->GetComputeFastNormals();
  }

  VISKORES_DEPRECATED(2.1, "Use SetComputeFastNormals.")
  VISKORES_CONT void SetComputeFastNormalsForUnstructured(bool on)
  {
    this->SetComputeFastNormals(on);
  }
  VISKORES_DEPRECATED(2.1, "Use GetComputeFastNormals.")
  VISKORES_CONT bool GetComputeFastNormalsForUnstructured() const
  {
    return this->GetComputeFastNormals();
  }

protected:
  // Needed by the subclass Slice
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& result) override;
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_Contour_h
