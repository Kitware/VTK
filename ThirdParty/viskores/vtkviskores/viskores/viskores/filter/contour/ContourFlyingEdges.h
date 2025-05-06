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

#ifndef viskores_filter_contour_ContourFlyingEdges_h
#define viskores_filter_contour_ContourFlyingEdges_h

#include <viskores/filter/contour/AbstractContour.h>
#include <viskores/filter/contour/viskores_filter_contour_export.h>

namespace viskores
{
namespace filter
{
namespace contour
{
/// \brief generate isosurface(s) from a 3-dimensional structured mesh

/// Takes as input a 3D structured mesh and generates on
/// output one or more isosurfaces using the Flying Edges algorithm.
/// Multiple contour values must be specified to generate the isosurfaces.
///
/// This implementation only accepts \c CellSetStructured<3> inputs using
/// \c ArrayHandleUniformPointCoordinates for point coordinates,
/// and is only used as part of the more general \c Contour filter
class VISKORES_FILTER_CONTOUR_EXPORT ContourFlyingEdges
  : public viskores::filter::contour::AbstractContour
{
protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& result) override;
};
} // namespace contour
} // namespace filter
} // namespace viskores

#endif // viskores_filter_contour_ContourFlyingEdges_h
