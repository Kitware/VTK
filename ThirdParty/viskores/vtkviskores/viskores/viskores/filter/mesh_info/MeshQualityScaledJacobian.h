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
#ifndef viskores_filter_mesh_info_MeshQualityScaledJacobian_h
#define viskores_filter_mesh_info_MeshQualityScaledJacobian_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute for each cell a metric derived from the Jacobian matric with normalization involving edge length.
///
/// This only produces values for triangles, quadrilaterals, tetrahedra, and hexahedra.
///
/// For a triangle, an acceptable range for good quality is [0.5, 2*sqrt(3)/3]. The value for
/// an equalateral triangle is 1. The normal range is [-2*sqrt(3)/3), 2*sqrt(3)/3], but
/// malformed cells can have plus or minus the maximum float value.
///
/// For a quadrilateral, an acceptable range for good quality is [0.3, 1]. The unit square
/// has a value of 1. The normal range as well as the full range is [-1, 1].
///
/// For a tetrahedron, an acceptable range for good quality is [0.5, sqrt(2)/2]. The value for
/// a unit equalateral triangle is 1. The normal range of values is [-sqrt(2)/2, sqrt(2)/2], but
/// malformed cells can have plus or minus the maximum float value.
///
/// For a hexahedron, an acceptable range for good quality is [0.5, 1]. The unit cube has
/// a value of 1. The normal range is [ -1, 1 ], but malformed cells can have a maximum float
/// value.
class VISKORES_FILTER_MESH_INFO_EXPORT MeshQualityScaledJacobian : public viskores::filter::Filter
{
public:
  MeshQualityScaledJacobian();

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_MeshQualityScaledJacobian_h
