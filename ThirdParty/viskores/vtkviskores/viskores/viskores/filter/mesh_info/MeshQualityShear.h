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
#ifndef viskores_filter_mesh_info_MeshQualityShear_h
#define viskores_filter_mesh_info_MeshQualityShear_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute the shear of each cell.
///
/// The shear of a cell is computed by taking the minimum of the Jacobian at each corner
/// divided by the length of the corner's adjacent edges.
///
/// This only produces values for quadrilaterals and hexahedra. Good quality cells will
/// have values in the range [0.3, 1]. Poorer quality cells can have values as low as 0.
class VISKORES_FILTER_MESH_INFO_EXPORT MeshQualityShear : public viskores::filter::Filter
{
public:
  MeshQualityShear();

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_MeshQualityShear_h
