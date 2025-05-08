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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_filter_mesh_info_MeshQualityRelativeSizeSquared_h
#define viskores_filter_mesh_info_MeshQualityRelativeSizeSquared_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute for each cell the ratio of area or volume to the mesh average.
///
/// If S is the size of a cell and avgS is the average cell size in the mesh, then
/// let R = S/avgS. R is "normalized" to be in the range [0, 1] by taking the minimum
/// of R and 1/R. This value is then squared.
///
/// This only produces values for triangles, quadrilaterals, tetrahedra, and hexahedra.
///
/// For a good quality triangle, the relative sized squared should be in the range [0.25, 1].
/// For a good quality quadrilateral, it should be in the range [0.3, 1].
/// For a good quality tetrahedron, it should be in the range [0.3, 1].
/// For a good quality hexahedron, it should be in the range [0.5, 1].
/// Poorer quality cells can have a relative size squared as low as 0.
class VISKORES_FILTER_MESH_INFO_EXPORT MeshQualityRelativeSizeSquared
  : public viskores::filter::Filter
{
public:
  MeshQualityRelativeSizeSquared();

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_MeshQualityRelativeSizeSquared_h
