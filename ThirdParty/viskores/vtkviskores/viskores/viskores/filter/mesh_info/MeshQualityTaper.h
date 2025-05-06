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
#ifndef viskores_filter_mesh_info_MeshQualityTaper_h
#define viskores_filter_mesh_info_MeshQualityTaper_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute the taper of each cell.
///
/// The taper of a quadrilateral is computed as the maximum ratio of the cross-derivative
/// with its shortest associated principal axis.
///
/// This only produces values for quadrilaterals and hexahedra.
///
/// A good quality quadrilateral will have a taper in the range of [0, 0.7]. A good quality
/// hexahedron will have a taper in the range of [0, 0.5]. The unit square or cube will
/// have a taper of 0. Poorer quality cells will have larger values (with no upper limit).
class VISKORES_FILTER_MESH_INFO_EXPORT MeshQualityTaper : public viskores::filter::Filter
{
public:
  MeshQualityTaper();

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_MeshQualityTaper_h
