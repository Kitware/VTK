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

#ifndef viskores_filter_contour_ContourMarchingCells_h
#define viskores_filter_contour_ContourMarchingCells_h

#include <viskores/filter/contour/AbstractContour.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// \brief generate isosurface(s) from a Volume using the Marching Cells algorithm
///
/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
///
/// This implementation is not optimized for all use cases, it is used by
/// the more general \c Contour filter which selects the best implementation
/// for all types of \c DataSet . .
class VISKORES_FILTER_CONTOUR_EXPORT ContourMarchingCells
  : public viskores::filter::contour::AbstractContour
{
protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& result) override;

  template <viskores::UInt8 Dims>
  VISKORES_CONT viskores::cont::DataSet DoExecuteDimension(const viskores::cont::DataSet& result);
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_ContourMarchingCells_h
